/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include "resolve.hh"
#include <optional>
#include <vector>
#include <map>


/* -------------------------------------------------------------------------- */

/* Exposed in Nix 2.15.x through `installable-flakes.hh',
 * but it's been defined for internal linkage long before that.
 * To avoid build failures with 2.13.x and later we just use `extern'. */
namespace nix {
  extern nix::ref<nix::eval_cache::EvalCache> openEvalCache(
    nix::EvalState                           & state
  , std::shared_ptr<nix::flake::LockedFlake>   lockedFlake
  );
}


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  static inline bool
shouldSearchSystem( std::string_view system )
{
  for ( const std::string & s : defaultSystems )
    {
      if ( s == system ) { return true; }
    }
  return false;
}


/* -------------------------------------------------------------------------- */

  static inline bool
isPkgsSubtree( std::string_view attrName )
{
  return ( attrName == "packages" ) ||
         ( attrName == "legacyPackages" ) ||
         ( attrName == "catalog"  );
}


/* -------------------------------------------------------------------------- */

static nix::flake::LockFlags floxFlakeLockFlags = {
  .updateLockFile = false
, .writeLockFile  = false
, .applyNixConfig = false
};


/* -------------------------------------------------------------------------- */

FloxFlakeRef coerceFlakeRef( std::string_view uri );
FloxFlakeRef coerceFlakeRef( FloxFlakeRef & ref );


/* -------------------------------------------------------------------------- */

std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
  nix::ref<nix::EvalState> state
, std::string_view         uri
);

std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
        nix::ref<nix::EvalState>   state
, const FloxFlakeRef             & ref
);

std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
  nix::ref<nix::EvalState>                   state
, std::shared_ptr<nix::flake::LockedFlake> & locked
);


/* -------------------------------------------------------------------------- */

nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
  nix::ref<nix::EvalState> state
, std::string_view         uri
);

nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
        nix::ref<nix::EvalState>   state
, const FloxFlakeRef             & ref
);

nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
  nix::ref<nix::EvalState>                   state
, std::shared_ptr<nix::flake::LockedFlake> & locked
);

nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
  nix::ref<nix::EvalState>               state
, nix::ref<nix::eval_cache::EvalCache> & cache
);


/* -------------------------------------------------------------------------- */

std::vector<nix::Symbol> coerceSymbols(
        nix::ref<nix::EvalState>        state
, const std::vector<std::string_view> & lst
);

std::vector<nix::Symbol> coerceSymbols(
  nix::ref<nix::EvalState>   state
, std::vector<nix::Symbol> & lst
);


/* -------------------------------------------------------------------------- */

std::vector<nix::SymbolStr> coerceSymbolStrs(
        nix::ref<nix::EvalState>        state
, const std::vector<std::string_view> & lst
);

std::vector<nix::SymbolStr> coerceSymbolStrs(
        nix::ref<nix::EvalState>   state
, const std::vector<nix::Symbol> & lst
);

std::vector<nix::SymbolStr> coerceSymbolStrs(
  nix::ref<nix::EvalState>      state
, std::vector<nix::SymbolStr> & lst
);


/* -------------------------------------------------------------------------- */

/**
 * Process `Inputs' arg using `Preferences' settings to sort inputs in
 * priority ordering.
 * In the process fetch and lock inputs to prepare them for evaluation.
 */
std::map<std::string, std::shared_ptr<nix::flake::LockedFlake>> prepInputs(
        nix::ref<nix::EvalState>   state
, const Inputs                   & inputs
, const Preferences              & prefs
);


/* -------------------------------------------------------------------------- */

std::vector<CursorPos> globSystems(
        nix::ref<nix::EvalState>   state
,       CursorPos                & c
, const std::list<std::string>   & systems = defaultSystems
);

std::vector<CursorPos> globSystems(
        nix::ref<nix::EvalState> state
,       std::vector<CursorPos>   & cs
, const std::list<std::string>   & systems = defaultSystems
);


/* -------------------------------------------------------------------------- */

bool sortByDepth( const AttrPathGlob & a, const AttrPathGlob & b ) noexcept;


/* -------------------------------------------------------------------------- */

nix::Value * loadFlakeRoot(
  nix::ref<nix::EvalState>                 state
, std::shared_ptr<nix::flake::LockedFlake> lockedFlake
);


/* -------------------------------------------------------------------------- */

template <typename T, template <typename, typename> class C>
  static inline bool
hasElement( const C<T, std::allocator<T>> & container, const T & e )
{
  return container.find( e ) != container.end();
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
