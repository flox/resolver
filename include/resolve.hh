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
#include "descriptor.hh"


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

  Preferences( const nlohmann::json & desc );

  nlohmann::json toJSON() const;
};


void from_json( const nlohmann::json & j,       Preferences & p );
void to_json(         nlohmann::json & j, const Preferences & p );


/* -------------------------------------------------------------------------- */

struct Resolved {
  FloxInput              input;
  std::list<std::string> path;
  std::string            uri;
  nlohmann::json         info;

  Resolved( const std::string_view   desc );
  Resolved( const nlohmann::json   & desc );

  nlohmann::json toJSON()   const;
  std::string    toString() const;
};


void from_json( const nlohmann::json & j,       Resolved & p );
void to_json(         nlohmann::json & j, const Resolved & p );


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
