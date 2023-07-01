/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>
#include <iostream>
#include <optional>
#include "flox/raw-package-set.hh"
// #include "flox/flake-package-set.hh"
// #include "flox/db-package-set.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

static const std::string nixpkgsRef =
  "github:NixOS/nixpkgs/e8039594435c68eb4f780f3e9bf3972a7399c4b1";


/* -------------------------------------------------------------------------- */

  bool
test_RawPackageSet_iterator1()
{
  CachedPackageMap pkgs {
    { { "hello" }
    , nix::make_ref<CachedPackage>(
        (std::vector<std::string_view>) { "hello" }
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

  RUN_TEST( RawPackageSet_iterator1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
