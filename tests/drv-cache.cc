/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>
#include <iostream>
#include "resolve.hh"
#include "flox/drv-cache.hh"
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;

/* -------------------------------------------------------------------------- */

static const std::string nixpkgsRef =
  "github:NixOS/nixpkgs/e8039594435c68eb4f780f3e9bf3972a7399c4b1";


/* -------------------------------------------------------------------------- */

/* Ensure name resolution works. */
  bool
test_getProgress1()
{
  Inputs        inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences   prefs;
  ResolverState rs( inputs, prefs );
  std::optional<nix::ref<FloxFlake>> mf    = rs.getInput( "nixpkgs" );
  nix::ref<FloxFlake>                flake = mf.value();
  DrvDb cache( flake->getLockedFlake()->getFingerprint() );
  // TODO: you need a phony temp DB.
  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_getDrvInfo1()
{
  Inputs        inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences   prefs;
  ResolverState rs( inputs, prefs );
  std::optional<nix::ref<FloxFlake>> mf    = rs.getInput( "nixpkgs" );
  nix::ref<FloxFlake>                flake = mf.value();
  DrvDb cache( flake->getLockedFlake()->getFingerprint() );
  std::optional<nlohmann::json> _info = cache.getDrvInfo(
    "legacyPackages", "x86_64-linux", { "hello" }
  );
  if ( ! _info.has_value() ) { return false; }
  nlohmann::json info = _info.value();
  return info.at( "pname" ) == "hello";
}


/* -------------------------------------------------------------------------- */

  bool
test_getDrvInfos1()
{
  Inputs        inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences   prefs;
  ResolverState rs( inputs, prefs );
  std::optional<nix::ref<FloxFlake>> mf    = rs.getInput( "nixpkgs" );
  nix::ref<FloxFlake>                flake = mf.value();
  DrvDb cache( flake->getLockedFlake()->getFingerprint() );
  std::list<nlohmann::json> infos = cache.getDrvInfos(
    "legacyPackages", "x86_64-linux"
  );
  /* While I don't like hard coding this, we explicitly want to ensure we aren't
   * off by one and dropping the "first" or "last" query result.
   * Because we are using a pinned flake this shouldn't be a real issue. */
  return infos.size() == 64037;
}


/* -------------------------------------------------------------------------- */

  bool
test_CachedPackageFromDb1()
{
  Inputs        inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences   prefs;
  ResolverState rs( inputs, prefs );
  std::optional<nix::ref<FloxFlake>> mf    = rs.getInput( "nixpkgs" );
  nix::ref<FloxFlake>                flake = mf.value();
  DrvDb cache( flake->getLockedFlake()->getFingerprint() );
  CachedPackage p( cache, "legacyPackages", "x86_64-linux", { "hello" } );
  return p.getPname() == "hello";
}


/* -------------------------------------------------------------------------- */

  bool
test_CachedPackageFromDb2()
{
  Inputs        inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences   prefs;
  ResolverState rs( inputs, prefs );
  std::optional<nix::ref<FloxFlake>> mf    = rs.getInput( "nixpkgs" );
  nix::ref<FloxFlake>                flake = mf.value();
  DrvDb cache( flake->getLockedFlake()->getFingerprint() );
  CachedPackage p( cache, "legacyPackages", "x86_64-linux", { "hello" } );
  Descriptor desc( (nlohmann::json) { { "name", "hello" } } );
  predicates::PkgPred pred = prefs.pred_V2();
  pred = pred && desc.pred( true );
  return pred( p );
}


/* -------------------------------------------------------------------------- */

  bool
test_CachedPackageFromInfo1()
{
  Inputs        inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences   prefs;
  ResolverState rs( inputs, prefs );
  std::optional<nix::ref<FloxFlake>> mf    = rs.getInput( "nixpkgs" );
  nix::ref<FloxFlake>                flake = mf.value();
  DrvDb cache( flake->getLockedFlake()->getFingerprint() );
  std::optional<nlohmann::json> _info = cache.getDrvInfo(
    "legacyPackages", "x86_64-linux", { "hello" }
  );
  CachedPackage p0( cache, "legacyPackages", "x86_64-linux", { "hello" } );
  CachedPackage p1( _info.value() );
  bool rsl = true;
  rsl &= p0.getPathStrs() == p1.getPathStrs();
  rsl &= p0.getFullName() == p1.getFullName();
  rsl &= p0.getPname() == p1.getPname();
  rsl &= p0.getVersion() == p1.getVersion();
  rsl &= p0.getSemver() == p1.getSemver();
  rsl &= p0.getLicense() == p1.getLicense();
  rsl &= p0.getOutputs() == p1.getOutputs();
  rsl &= p0.getOutputsToInstall() == p1.getOutputsToInstall();
  rsl &= p0.isBroken().has_value() && p1.isBroken().has_value();
  rsl &= p0.isBroken().value() == p1.isBroken().value();
  rsl &= p0.isUnfree().has_value() && p1.isUnfree().has_value();
  rsl &= p0.isUnfree().value() == p1.isUnfree().value();
  rsl &= p0.hasMetaAttr() == p1.hasMetaAttr();
  rsl &= p0.hasPnameAttr() == p1.hasPnameAttr();
  rsl &= p0.hasVersionAttr() == p1.hasVersionAttr();
  return rsl;
}


/* -------------------------------------------------------------------------- */

#define RUN_TEST( _NAME )                                              \
  try                                                                  \
    {                                                                  \
      if ( ! test_ ## _NAME () )                                       \
        {                                                              \
          ec = EXIT_FAILURE;                                           \
          std::cerr << "  fail: " # _NAME << std::endl;                \
        }                                                              \
    }                                                                  \
  catch( std::exception & e )                                          \
    {                                                                  \
      ec = EXIT_FAILURE;                                               \
      std::cerr << "  ERROR: " # _NAME ": " << e.what() << std::endl;  \
    }


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;

  /* Ensure that database is initialized */
  Inputs        inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences   prefs;
  ResolverState rs( inputs, prefs );
  Descriptor    desc( (nlohmann::json) { { "name", "hello" } } );
  rs.resolveInInput( "nixpkgs", desc );

  RUN_TEST( getProgress1 );
  RUN_TEST( getDrvInfo1 );
  RUN_TEST( getDrvInfos1 );
  RUN_TEST( CachedPackageFromDb1 );
  RUN_TEST( CachedPackageFromDb2 );
  RUN_TEST( CachedPackageFromInfo1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
