/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>
#include <iostream>
#include <optional>
#include "flox/raw-package-set.hh"
#include "flox/types.hh"
#include "descriptor.hh"
#include "flox/db-package-set.hh"
#include "flox/flake-package-set.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

static const std::string nixpkgsRef =
  "github:NixOS/nixpkgs/e8039594435c68eb4f780f3e9bf3972a7399c4b1";

/**
 * These counts indicate the total number of derivations under
 * `<nixpkgsRef>#legacyPackages.x86_64-linux.**' which we will use to sanity
 * check calls to `size()'.
 * Note that the legacy implementation used to populate `DbPackageSet' will
 * fail to evaluate 3 packages which require `NIXPKGS_ALLOW_BROKEN', causing
 * different sizes to be collected ( until migration is coompleted ).
 */
static const size_t unbrokenPkgCount = 64037;
static const size_t fullPkgCount     = 64040;


/* -------------------------------------------------------------------------- */

  bool
test_RawPackageSet_iterator1()
{
  RawPackageMap pkgs {
    { { "hello" }
    , nix::make_ref<RawPackage>(
        (std::vector<std::string_view>) {
          "legacyPackages", "x86_64-linux", "hello"
        }
      , "hello-2.12.1"
      , "hello"
      , "2.12.1"
      , "2.12.1"
      , "GPL-3.0-or-later"
      , (std::vector<std::string_view>) { "out" }
      , (std::vector<std::string_view>) { "out" }
      , std::make_optional( false )
      , std::make_optional( false )
      , true
      , true
      , true
      )
    }
  };
  RawPackageSet ps {
    std::move( pkgs )
  , ST_LEGACY
  , "x86_64-linux"
  , std::nullopt
  , nix::parseFlakeRef( nixpkgsRef )
  };

  for ( auto it = ps.begin(); it != ps.end(); ++it )
    {
      if ( ( * it ).getPname() != "hello" ) { return false; }
      break;
    }
  for ( auto & p : ps )
    {
      if ( p.getPname() != "hello" ) { return false; }
      break;
    }

  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_RawPackageSet_addPackage1()
{
  RawPackage pkg(
    (std::vector<std::string_view>) {
      "legacyPackages", "x86_64-linux", "hello"
    }
  , "hello-2.12.1"
  , "hello"
  , "2.12.1"
  , "2.12.1"
  , "GPL-3.0-or-later"
  , (std::vector<std::string_view>) { "out" }
  , (std::vector<std::string_view>) { "out" }
  , std::make_optional( false )
  , std::make_optional( false )
  , true
  , true
  , true
  );
  RawPackageSet ps {
    {}
  , ST_LEGACY
  , "x86_64-linux"
  , std::nullopt
  , nix::parseFlakeRef( nixpkgsRef )
  };

  ps.addPackage( std::move( pkg ) );

  for ( auto it = ps.begin(); it != ps.end(); ++it )
    {
      if ( ( * it ).getPname() != "hello" ) { return false; }
      break;
    }
  for ( auto & p : ps )
    {
      if ( p.getPname() != "hello" ) { return false; }
      break;
    }

  return true;
}


/* -------------------------------------------------------------------------- */

  bool
test_DbPackageSet_iterator1( std::shared_ptr<nix::flake::LockedFlake> flake )
{
  DbPackageSet ps( flake, ST_LEGACY, "x86_64-linux" );
  size_t c1 = 0;
  size_t c2 = 0;
  for ( auto it = ps.begin(); it != ps.end(); ++it, ++c1 ) {}
  for ( auto & p : ps ) { ++c2; }
  // TODO: after DB is populated by new routines change to `fullPkgCount'
  return ( c1 == c2 ) && ( c1 == unbrokenPkgCount );
}


/* -------------------------------------------------------------------------- */

  bool
test_DbPackageSet_size1( std::shared_ptr<nix::flake::LockedFlake> flake )
{
  DbPackageSet ps( flake, ST_LEGACY, "x86_64-linux" );
  size_t c = 0;
  for ( auto & p : ps ) { ++c; }
  // TODO: after DB is populated by new routines change to `fullPkgCount'
  return ( c == ps.size() ) && ( c == unbrokenPkgCount );
}


/* -------------------------------------------------------------------------- */

  bool
test_FlakePackageSet_size1(
  ResolverState                            & rs
, std::shared_ptr<nix::flake::LockedFlake>   flake
)
{
  FlakePackageSet ps( rs.getEvalState(), flake, ST_LEGACY, "x86_64-linux" );
  return ps.size() == fullPkgCount;
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

#define RUN_TEST_WITH_FLAKE( _FLAKE, _NAME )                           \
  try                                                                  \
    {                                                                  \
      if ( ! test_ ## _NAME ( _FLAKE ) )                               \
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

#define RUN_TEST_WITH_STATE_FLAKE( _STATE, _FLAKE, _NAME )             \
  try                                                                  \
    {                                                                  \
      if ( ! test_ ## _NAME ( _STATE, _FLAKE ) )                       \
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

  RUN_TEST( RawPackageSet_iterator1 );
  RUN_TEST( RawPackageSet_addPackage1 );

  std::shared_ptr<nix::flake::LockedFlake> flake  = nullptr;
  Inputs      inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs;
  ResolverState rs(
    inputs
  , prefs
  , (std::list<std::string>) { "x86_64-linux" }
  );
  {
    /* Initialize DB */
    Descriptor d( (nlohmann::json) { { "name", "hello" } } );
    rs.resolveInInput( "nixpkgs", d );

    nix::ref<FloxFlake> nixpkgs = rs.getInput( "nixpkgs" ).value();
    flake = nixpkgs->getLockedFlake();
  }

  RUN_TEST_WITH_FLAKE( flake, DbPackageSet_iterator1 );
  RUN_TEST_WITH_FLAKE( flake, DbPackageSet_size1 );

  RUN_TEST_WITH_STATE_FLAKE( rs, flake, FlakePackageSet_size1 );

  // FIXME: `FlakePackageSet::begin()'

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
