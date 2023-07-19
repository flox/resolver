/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <iterator>
#include <cstddef>
#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <nix/eval-cache.hh>
#include <unordered_map>
#include <unordered_set>
#include "flox/exceptions.hh"
#include "flox/util.hh"
#include <queue>
#include <any>
#include "flox/types.hh"
#include "flox/flox-flake.hh"
#include "flox/resolved.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class ResolverState {
  private:
    std::shared_ptr<nix::Store>                       _store;
    std::shared_ptr<nix::Store>                       evalStore;
    std::shared_ptr<nix::EvalState>                   evalState;
    std::map<std::string, std::shared_ptr<FloxFlake>> _inputs;
    const Preferences                                 _prefs;

  public:

    nix::ref<nix::Store>       getStore();
    nix::ref<nix::Store>       getEvalStore();
    nix::ref<nix::EvalState>   getEvalState();
    nix::SymbolTable         * getSymbolTable();

    ResolverState( const Inputs                 & inputs
                 , const Preferences            & prefs
                 , const std::list<std::string> & systems = defaultSystems
                 );

    Preferences getPreferences() const { return this->_prefs; }

    std::map<std::string, nix::ref<FloxFlake>> getInputs() const;
    std::list<std::string_view>                getInputNames() const;

    std::optional<nix::ref<FloxFlake>> getInput( std::string_view id ) const;

    std::list<Resolved> resolveInInput(       std::string_view   id
                                      , const Descriptor       & desc
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
