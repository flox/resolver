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
#include "flox/exceptions.hh"
#include "flox/types.hh"
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
    /**
     * "hello"                  -> { name: "hello" }
     * "hello@18"               -> { name: "hello", semver: "18" }
     * "hello@=18"              -> { name: "hello", version: "18" }
     * "packages.*.hello"       -> { path: ["packages", null, "hello"] }
     * "nixpkgs#hello"          -> { flake.id: "nixpkgs", name: "hello" }
     * "catalog:floxpkgs#hello" -> { catalog.id: "floxpkgs", name: "hello" }
     */
    Descriptor( const std::string_view desc );

    predicates::PkgPred pred( nix::SymbolTable & st ) const;

    nlohmann::json toJSON()   const;
    std::string    toString() const;
};


void from_json( const nlohmann::json & j,       Descriptor & p );
void to_json(         nlohmann::json & j, const Descriptor & p );


/* -------------------------------------------------------------------------- */

class DescriptorFunctor {
  private:
          nix::ref<nix::EvalState>   state;
    const Preferences              * prefs;
    const Descriptor               * desc;
          PkgPredicate               prefsPredicate = defaultPkgPredicate;

  public:
    // TODO: make private
    std::unordered_map<AttrPathGlob, Resolved> results;

    DescriptorFunctor( nix::ref<nix::EvalState> & state
                     , const Preferences        & prefs
                     , const Descriptor         & desc
                     )
      : state( state )
      , prefs( & prefs )
      , desc( & desc )
      , prefsPredicate( prefs.pred() )
    {}

    /* std::vector<std::pair<Cursor, std::vector<nix::Symbol>>> */
    std::vector<CursorPos> getRoots(
      std::string_view                         inputId
    , std::shared_ptr<nix::flake::LockedFlake> flake
    );

    bool shouldRecur(       Cursor                     pos
                    , const std::vector<nix::Symbol> & path
                    );
    bool packagePredicate(       Cursor                     pos
                         , const std::vector<nix::Symbol> & path
                         );

    void addResult( const FloxFlakeRef                & ref
                  , const std::vector<nix::SymbolStr> & path
                  ,       std::string_view              name
                  ,       std::string_view              version
                  );

    void visit( const FloxFlakeRef             & ref
              ,       Cursor                     cur
              , const std::vector<nix::Symbol> & attrPath
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
