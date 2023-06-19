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
#include "flox/util.hh"


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
  Resolved r( ref, AttrPathGlob( { "packages", nullptr, "hello" } ), {} );
  return r.toString() == "github:NixOS/nixpkgs#packages.{{system}}.hello";
}


/* -------------------------------------------------------------------------- */

/* Immutable form. */
  bool
test_mergeResolvedByAttrPathGlob1()
{
  FloxFlakeRef ref = nix::parseFlakeRef( "github:NixOS/nixpkgs" );

  std::list<Resolved> lst;

  auto mkEnt = [&]( std::string && system, std::string && name, int v )
  {
    lst.emplace_back( Resolved(
      ref
    , AttrPathGlob::fromStrings( (std::vector<std::string>) {
        "packages", system, name
      } )
      , { { system, { { "foo", v } } } }
    ) );
  };
  mkEnt( "x86_64-linux",  "hello",  1 );
  mkEnt( "aarch64-linux", "hello",  2 );
  mkEnt( "x86_64-linux",  "cowsay", 3 );
  mkEnt( "aarch64-linux", "cowsay", 4 );

  std::list<Resolved> merged =
    mergeResolvedByAttrPathGlob( (const std::list<Resolved>) lst );
  return ( lst.size() == 4 )                 &&
         ( merged.size() == 2 )              &&
         ( merged.front().info.size() == 2 ) &&
         ( merged.back().info.size() == 2 );
}


/* -------------------------------------------------------------------------- */

/* Mutable form. */
  bool
test_mergeResolvedByAttrPathGlob2()
{
  FloxFlakeRef ref = nix::parseFlakeRef( "github:NixOS/nixpkgs" );

  std::list<Resolved> lst;

  auto mkEnt = [&]( std::string && system, std::string && name, int v )
  {
    lst.emplace_back( Resolved(
      ref
    , AttrPathGlob::fromStrings( (std::vector<std::string>) {
        "packages", system, name
      } )
      , { { system, { { "foo", v } } } }
    ) );
  };
  mkEnt( "x86_64-linux",  "hello",  1 );
  mkEnt( "aarch64-linux", "hello",  2 );
  mkEnt( "x86_64-linux",  "cowsay", 3 );
  mkEnt( "aarch64-linux", "cowsay", 4 );

  mergeResolvedByAttrPathGlob( lst );
  return ( lst.size() == 2 )              &&
         ( lst.front().info.size() == 2 ) &&
         ( lst.back().info.size() == 2 );
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
  RUN_TEST( ResolvedFromJSON1 );
  RUN_TEST( ResolvedToJSON1 );
  RUN_TEST( ResolvedToString );
  RUN_TEST( mergeResolvedByAttrPathGlob1 );
  RUN_TEST( mergeResolvedByAttrPathGlob2 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
