/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "test.hh"
#include <nlohmann/json.hpp>
#include "resolve.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

  bool
test_PreferencesFromJSON1()
{
  nlohmann::json prefs = R"(
    {
      "inputs":   ["nixpkgs", "nixpkgs-flox"]
    , "allow":    { "unfree": false }
    , "semver":   { "preferPreReleases": true }
    , "prefixes": {
        "nixpkgs":      ["legacyPackages", "packages", "catalog"]
      , "nixpkgs-flox": ["catalog", "packages", "legacyPackages"]
      }
    , "stabilities": { "nixpkgs-flox": ["unstable", "staging", "unstable"] }
    }
  )"_json;
  Preferences d( prefs );
  return ( d.inputs[0] == "nixpkgs" ) &&
         ( d.inputs[1] == "nixpkgs-flox" ) &&
         ( ! d.allowBroken ) &&
         ( ! d.allowUnfree ) &&
         d.semverPreferPreReleases &&
         ( d.prefixes["nixpkgs"][0]      == "legacyPackages" ) &&
         ( d.prefixes["nixpkgs-flox"][0] == "catalog" ) &&
         ( d.stabilities["nixpkgs-flox"][0] == "unstable" );
}

  bool
test_PreferencesToJSON1()
{
  nlohmann::json prefs = R"(
    {
      "inputs":   ["nixpkgs", "nixpkgs-flox"]
    , "allow":    { "unfree": false }
    , "semver":   { "preferPreReleases": true }
    , "prefixes": {
        "nixpkgs":      ["legacyPackages", "packages", "catalog"]
      , "nixpkgs-flox": ["catalog", "packages", "legacyPackages"]
      }
    , "stabilities": { "nixpkgs-flox": ["unstable", "staging", "unstable"] }
    }
  )"_json;
  Preferences    d( prefs );
  nlohmann::json j = d.toJSON();

  return
    ( prefs["inputs"] == j["inputs"] ) &&
    ( prefs["semver"] == j["semver"] ) &&
    ( prefs["prefixes"]["nixpkgs"] == j["prefixes"]["nixpkgs"] ) &&
    ( prefs["prefixes"]["nixpkgs-flox"] == j["prefixes"]["nixpkgs-flox"] ) &&
    ( prefs["stabilities"]["nixpkgs-flox"] ==
      j["stabilities"]["nixpkgs-flox"]
    ) &&
    ( prefs["allow"]["unfree"] == j["allow"]["unfree"] ) &&
    ( ! j["allow"]["broken"] );
}


/* -------------------------------------------------------------------------- */

  bool
test_PreferencesToJSON2()
{
  nlohmann::json prefs = {};
  Preferences d( prefs );
  nlohmann::json j = d.toJSON();

  return j["allow"]["unfree"] &&
         ( ! j["allow"]["broken"] ) &&
         ( ! j["semver"]["preferPreReleases"] );
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

  RUN_TEST( PreferencesFromJSON1 );
  RUN_TEST( PreferencesToJSON1 );
  RUN_TEST( PreferencesToJSON2 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
