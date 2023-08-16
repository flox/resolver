/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "test.hh"
#include "resolve.hh"
#include "flox/drv-cache.hh"
#include "flox/util.hh"
#include "flox/cached-package-set.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;

/* -------------------------------------------------------------------------- */

  bool
test_getDrvInfo1( DrvDb * cache )
{
  std::optional<nlohmann::json> _info = cache->getDrvInfo(
    "legacyPackages", "x86_64-linux", { "hello" }
  );
  if ( ! _info.has_value() ) { return false; }
  nlohmann::json info = _info.value();
  return info.at( "pname" ) == "hello";
}


/* -------------------------------------------------------------------------- */

  bool
test_getDrvInfos1( DrvDb * cache )
{
  std::list<nlohmann::json> infos = cache->getDrvInfos(
    "legacyPackages", "x86_64-linux"
  );
  /* While I don't like hard coding this, we explicitly want to ensure we aren't
   * off by one and dropping the "first" or "last" query result.
   * Because we are using a pinned flake this shouldn't be a real issue. */
  return infos.size() == unbrokenPkgCount;
}


/* -------------------------------------------------------------------------- */

  bool
test_CachedPackageFromDb1( DrvDb * cache )
{
  CachedPackage p( * cache, "legacyPackages", "x86_64-linux", { "hello" } );
  return p.getPname() == "hello";
}


/* -------------------------------------------------------------------------- */

  bool
test_CachedPackageFromDb2( DrvDb * cache, Preferences & prefs )
{
  CachedPackage p( * cache, "legacyPackages", "x86_64-linux", { "hello" } );
  Descriptor desc( nlohmann::json { { "name", "hello" } } );
  predicates::PkgPred pred = prefs.pred_V2();
  pred = pred && desc.pred( true );
  return pred( p );
}


/* -------------------------------------------------------------------------- */

  bool
test_CachedPackageFromInfo1( DrvDb * cache )
{
  std::optional<nlohmann::json> _info = cache->getDrvInfo(
    "legacyPackages", "x86_64-linux", { "hello" }
  );
  CachedPackage p0( * cache, "legacyPackages", "x86_64-linux", { "hello" } );
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

  int
main()
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

  /* Ensure that database is initialized */
  Inputs          inputs( nlohmann::json { { "nixpkgs", nixpkgsRef } } );
  Preferences     prefs;
  ResolverState   rs( inputs, prefs );
  DrvDb         * cache = nullptr;
  {
    std::shared_ptr<nix::flake::LockedFlake> flake  =
      rs.getInput( "nixpkgs" ).value()->getLockedFlake();
    {
      FlakePackageSet fps( rs.getEvalState()
                         , flake
                         , ST_LEGACY
                         , "x86_64-linux"
                         );
      cachePackageSet( fps );
    }
    cache = new DrvDb( flake->getFingerprint() );
  }

  RUN_TEST( getDrvInfo1, cache );
  RUN_TEST( getDrvInfos1, cache );
  RUN_TEST( CachedPackageFromDb1, cache );
  RUN_TEST( CachedPackageFromDb2, cache, prefs );
  RUN_TEST( CachedPackageFromInfo1, cache );

  delete cache;

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
