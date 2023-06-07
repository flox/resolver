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
#include <variant>
#include "flox/exceptions.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

typedef std::variant<std::nullptr_t, std::string>  attr_part;


/* -------------------------------------------------------------------------- */

class Descriptor {
  private:
    /* Check validity of fields, possibly returning an error message. */
    bool audit( std::string & msg ) const;

  public:
    /* ["python3", "pkgs", "pip"] */
    std::optional<std::vector<std::string>> relAttrPath;
    /* ["packages", null, "hello"] */
    std::optional<std::vector<attr_part>> absAttrPath;

    std::optional<std::string> name;
    std::optional<std::string> version;
    std::optional<std::string> semver;

    std::optional<std::string> catalogStability;

    bool searchCatalogs;
    bool searchFlakes;

    std::optional<std::string> inputId;

    Descriptor( const nlohmann::json   & desc );
    /**
     * "hello"                  -> { name: "hello" }
     * "hello@18"               -> { name: "hello", semver: "18" }
     * "hello@=18"              -> { name: "hello", version: "18" }
     * "packages.*.hello"       -> { path: ["packages", null, "hello"] }
     * "nixpkgs#hello"          -> { flake.id: "nixpkgs", name: "hello" }
     * "catalog:floxpkgs#hello" -> { catalog.id: "floxpkgs", name: "hello" }
     */
    Descriptor( const std::string_view   desc );

    nlohmann::json toJSON()   const;
    std::string    toString() const;
};


void from_json( const nlohmann::json & j,       Descriptor & p );
void to_json(         nlohmann::json & j, const Descriptor & p );


/* -------------------------------------------------------------------------- */

class Preferences;


class DescriptorFunctor {
  private:
    Descriptor * descriptor;

  public:
    bool shouldRecur(       nix::EvalState              & state
                    , const Preferences                 & prefs
                    ,       nix::eval_cache::AttrCursor & pos
                    , const std::vector<nix::Symbol>    & path
                    );
    bool packagePredicate(       nix::EvalState              & state
                         , const Preferences                 & prefs
                         , const nix::eval_cache::AttrCursor & pos
                         , const std::vector<nix::Symbol>    & path
                         );
};


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
