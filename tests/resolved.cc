/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "test.hh"
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
        "id": "nixpkgs-flox"
      , "locked": {
          "type":  "github"
        , "owner": "flox"
        , "repo":  "nixpkgs-flox"
        , "rev":   "d5b4fa110c4b546b7ed51a02d523973a8e075159"
        }
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
        "id": "nixpkgs-flox"
      , "locked": {
          "type":  "github"
        , "owner": "flox"
        , "repo":  "nixpkgs-flox"
        , "rev":   "d5b4fa110c4b546b7ed51a02d523973a8e075159"
        }
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
  Resolved r( "nixpkgs"
            , ref
            , AttrPathGlob( { "packages", nullptr, "hello" } )
            , {}
            );
  return r.toString() == "github:NixOS/nixpkgs#packages.{{system}}.hello";
}


/* -------------------------------------------------------------------------- */

  bool
test_mergeResolvedByAttrPathGlob1()
{
  FloxFlakeRef ref = nix::parseFlakeRef( "github:NixOS/nixpkgs" );

  std::list<Resolved> lst;

  auto mkEnt = [&]( std::string && system, std::string && name, int v )
  {
    lst.emplace_back( Resolved(
      "nixpkgs"
    , ref
    , AttrPathGlob::fromStrings( std::vector<std::string> {
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

  int
main()
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

  RUN_TEST( ResolvedFromJSON1 );
  RUN_TEST( ResolvedToJSON1 );
  RUN_TEST( ResolvedToString );
  RUN_TEST( mergeResolvedByAttrPathGlob1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
