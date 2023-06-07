/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>
#include <iostream>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include "resolve.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

  bool
test_ResolvedFromJSON1()
{
  nlohmann::json resolved = R"(
    {
      "input": {
        "type":  "github"
      , "owner": "flox"
      , "repo":  "nixpkgs-flox"
      , "rev":   "d5b4fa110c4b546b7ed51a02d523973a8e075159"
      }
    , "path": ["catalog", null, "stable", "hello", "2_12_1"]
    , "uri": "github:flox/nixpkgs-flox/d5b4fa110c4b546b7ed51a02d523973a8e075159#catalog.{{system}}.stable.hello.2_12_1"
    , "info": {
        "name": "hello"
      , "version": "2.12.1"
      , "semver": "2.12.1"
      , "systems": [
          "x86_64-linux"
        , "aarch64-linux"
        , "x86_64-darwin"
        , "aarch64-darwin"
        ]
      }
    }
  )"_json;
  Resolved d( resolved );
  return true;
}

  bool
test_ResolvedToJSON1()
{
  nlohmann::json resolved = R"(
    {
      "input": {
        "type":  "github"
      , "owner": "flox"
      , "repo":  "nixpkgs-flox"
      , "rev":   "d5b4fa110c4b546b7ed51a02d523973a8e075159"
      }
    , "path": ["catalog", null, "stable", "hello", "2_12_1"]
    , "uri": "github:flox/nixpkgs-flox/d5b4fa110c4b546b7ed51a02d523973a8e075159#catalog.{{system}}.stable.hello.2_12_1"
    , "info": {
        "name": "hello"
      , "version": "2.12.1"
      , "semver": "2.12.1"
      , "systems": [
          "x86_64-linux"
        , "aarch64-linux"
        , "x86_64-darwin"
        , "aarch64-darwin"
        ]
      }
    }
  )"_json;
  Resolved d( resolved );
  nlohmann::json j = d.toJSON();
  return resolved == j;
}


/* -------------------------------------------------------------------------- */

  bool
test_ResolvedToString()
{
  FloxFlakeRef ref = nix::parseFlakeRef( "github:NixOS/nixpkgs" );
  Resolved r( ref, { "packages", nullptr, "hello" }, {} );
  return r.toString() == "github:NixOS/nixpkgs#packages.{{system}}.hello";
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


  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;
  RUN_TEST( ResolvedFromJSON1 );
  RUN_TEST( ResolvedToJSON1 );
  RUN_TEST( ResolvedToString );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
