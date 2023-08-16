/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include <nix/fetchers.hh>
#include <unordered_map>
#include <unordered_set>
#include <nix/eval-cache.hh>
#include "flox/resolver/exceptions.hh"
#include "flox/resolver/types.hh"
#include "flox/predicates.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class Descriptor {
  private:
    /* Check validity of fields, possibly returning an error message. */
    bool audit( std::string & msg ) const;

  public:
    /* ["python3", "pkgs", "pip"] */
    std::optional<std::vector<std::string>> relAttrPath;
    /* ["packages", null, "hello"] */
    std::optional<AttrPathGlob> absAttrPath;

    std::optional<std::string> name;
    std::optional<std::string> version;
    std::optional<std::string> semver;

    std::optional<std::string> catalogStability;

    bool searchCatalogs = true;
    bool searchFlakes   = true;

    std::optional<std::string> inputId;

    Descriptor() = default;
    Descriptor( const nlohmann::json & desc );
    // TODO
    /* *
     * "hello"                  -> { name: "hello" }
     * "hello@18"               -> { name: "hello", semver: "18" }
     * "hello@=18"              -> { name: "hello", version: "18" }
     * "packages.*.hello"       -> { path: ["packages", null, "hello"] }
     * "nixpkgs#hello"          -> { flake.id: "nixpkgs", name: "hello" }
     * "catalog:floxpkgs#hello" -> { catalog.id: "floxpkgs", name: "hello" }
     */
    //Descriptor( const std::string_view desc );

    predicates::PkgPred pred( bool checkPath = false ) const;

    nlohmann::json toJSON()   const;
    std::string    toString() const;
};


void from_json( const nlohmann::json & j,       Descriptor & p );
void to_json(         nlohmann::json & j, const Descriptor & p );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
