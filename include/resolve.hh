/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <unordered_map>
#include <unordered_set>
#include "flox/exceptions.hh"
#include "descriptor.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

typedef nix::FlakeRef  FloxFlakeRef;


/* -------------------------------------------------------------------------- */

class Inputs {
  private:
    std::unordered_map<std::string, FloxFlakeRef> inputs;

    void init( const nlohmann::json & j );

  public:
    Inputs( const nlohmann::json & j ) { this->init( j ); }

    bool           has( std::string_view id ) const;
    FloxFlakeRef   get( std::string_view id ) const;
    nlohmann::json toJSON()                   const;
};


void from_json( const nlohmann::json & j,       Inputs & i );
void to_json(         nlohmann::json & j, const Inputs & i );


/* -------------------------------------------------------------------------- */

static const std::vector<std::string> defaultCatalogStabilities = {
  "stable", "staging", "unstable"
};

static const std::vector<std::string> defaultAttrPathPrefixes = {
  "catalog", "packages", "legacyPackages"
};


struct Preferences {
  std::vector<std::string> inputs;

  std::unordered_map<std::string, std::vector<std::string>> stabilities;
  std::unordered_map<std::string, std::vector<std::string>> prefixes;

  bool semverPreferPreReleases = false;
  bool allowUnfree             = true;
  bool allowBroken             = false;

  std::optional<std::unordered_set<std::string>> allowedLicenses;

  Preferences( const nlohmann::json & j );

  nlohmann::json toJSON() const;
};


void from_json( const nlohmann::json & j,       Preferences & p );
void to_json(         nlohmann::json & j, const Preferences & p );


/* -------------------------------------------------------------------------- */

class Resolved {
  private:
    FloxFlakeRef             input;
    std::vector<std::string> path;
    std::string              uri;
    nlohmann::json           info;

  public:
    Resolved(       std::string_view   uri );
    Resolved( const nlohmann::json   & attrs );

    nlohmann::json toJSON()   const;
    std::string    toString() const;
};


void from_json( const nlohmann::json & j,       Resolved & p );
void to_json(         nlohmann::json & j, const Resolved & p );


/* -------------------------------------------------------------------------- */

/* Return a ranked vector of satisfactory resolutions. */
std::vector<Resolved> resolve( const Inputs      & inputs
                             , const Preferences & preferences
                             , const Descriptor  & desc
                             );

/* Return the highest ranking resolution, or `std::nullopt'. */
std::optional<Resolved> resolveOne( const Inputs      & inputs
                                  , const Preferences & preferences
                                  , const Descriptor  & desc
                                  );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
