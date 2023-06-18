/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>
#include <iostream>
#include "resolve.hh"
#include "flox/drv-cache.hh"


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
  return infos.size() == 46535;
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
  std::cerr << p.getInfo().dump() << std::endl;
  return p.getPname() == "hello";
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

  RUN_TEST( getProgress1 );
  RUN_TEST( getDrvInfo1 );
  RUN_TEST( getDrvInfos1 );
  RUN_TEST( CachedPackageFromDb1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
