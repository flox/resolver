/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <list>
#include <optional>
#include <nlohmann/json.hpp>
#include <nix/fetchers.hh>
#include <unordered_map>
#include <unordered_set>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

struct FloxInput : public nix::fetchers::Input {
  std::optional<bool> catalog;
};


/* -------------------------------------------------------------------------- */

static const std::list<std::string> defaultCatalogStabilities = {
  "stable", "staging", "unstable"
};

static const std::list<std::string> defaultAttrPathPrefixes = {
  "catalog", "packages", "legacyPackages"
};

struct Preferences {
  std::list<std::string> inputs = { "nixpkgs", "nixpkgs-flox" };

  std::unordered_map<std::string, std::list<std::string>> catalogStabilities;
  std::unordered_map<std::string, std::list<std::string>> attrPathPrefixes;

  bool semverPreferPreReleases = false;
  bool allowUnfree             = true;
  bool allowBroken             = false;

  std::optional<std::unordered_set<std::string>> allowedLicenses;
};



/* -------------------------------------------------------------------------- */

struct Descriptor {
  std::optional<std::list<std::string>> path;
  std::optional<std::string>            name;
  std::optional<std::string>            version;
  std::optional<std::string>            semver;

  bool                       searchCatalogs;
  std::optional<std::string> catalogId;
  std::optional<std::string> catalogStability;

  bool                       searchFlakes;
  std::optional<std::string> flakeId;
};


/* -------------------------------------------------------------------------- */

struct Resolved {
  /* unsigned int           rank; */
  FloxInput              input;
  std::list<std::string> path;
  std::string            uri;
  nlohmann::json         info;
};


/* -------------------------------------------------------------------------- */

/* Return a ranked list of satisfactory resolutions. */
std::list<Resolved> resolve( const std::list<FloxInput> & inputs
                           , const Preferences          & preferences
                           , const Descriptor           & desc
                           );

/* Return the highest ranking resolution, or `std::nullopt'. */
std::optional<Resolved> resolveOne( const std::list<FloxInput> & inputs
                                  , const Preferences          & preferences
                                  , const Descriptor           & desc
                                  );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
