/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <unordered_map>
#include <unordered_set>
#include "flox/exceptions.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

typedef nix::FlakeRef  FloxFlakeRef;

typedef std::variant<std::nullptr_t, std::string>  attr_part;


/* -------------------------------------------------------------------------- */

struct AttrPathGlob {

  std::vector<attr_part> path;

  AttrPathGlob() = default;
  AttrPathGlob( const std::vector<attr_part>        & path );
  AttrPathGlob(       std::vector<std::string_view>   path );
  AttrPathGlob( const nlohmann::json                & path );

  std::string    toString() const;
  nlohmann::json toJSON()   const;

  bool isAbsolute() const;
  bool hasGlob()    const;
  /* Replace second element ( if present ) with `nullptr' glob. */
  void coerceRelative();
  /* Replace second element ( if present ) with `nullptr' glob. */
  void coerceGlob();

  bool globEq( const AttrPathGlob & other ) const;

};

void from_json( const nlohmann::json & j,       AttrPathGlob & path );
void to_json(         nlohmann::json & j, const AttrPathGlob & path );


/* -------------------------------------------------------------------------- */

class Inputs {
  private:
    void init( const nlohmann::json & j );

  public:
    std::unordered_map<std::string, FloxFlakeRef> inputs;

    Inputs() = default;
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

  Preferences() = default;
  Preferences( const nlohmann::json & j );

  nlohmann::json toJSON() const;
};


void from_json( const nlohmann::json & j,       Preferences & p );
void to_json(         nlohmann::json & j, const Preferences & p );


/* -------------------------------------------------------------------------- */

class Resolved {
  private:
    std::string  uri;
    FloxFlakeRef input;

  public:
    AttrPathGlob   path;
    nlohmann::json info;

    Resolved( const nlohmann::json & attrs );
    Resolved( const FloxFlakeRef   & input
            , const AttrPathGlob   & path
            , const nlohmann::json & info
            );

    nlohmann::json toJSON()   const;
    std::string    toString() const { return this->uri; }
};


void from_json( const nlohmann::json & j,       Resolved & p );
void to_json(         nlohmann::json & j, const Resolved & p );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
