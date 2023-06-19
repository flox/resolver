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
isPkgsSubtree( std::string_view attrName )
{
  return ( attrName == "legacyPackages" ) ||
         ( attrName == "packages" )       ||
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


/* -------------------------------------------------------------------------- */

std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
        nix::ref<nix::EvalState>   state
, const FloxFlakeRef             & ref
);

std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
  nix::ref<nix::EvalState> state
, std::string_view         uri
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


/* -------------------------------------------------------------------------- */

std::vector<nix::Symbol> coerceSymbols(
        nix::ref<nix::EvalState>        state
, const std::vector<std::string_view> & lst
);


/* -------------------------------------------------------------------------- */

std::vector<nix::SymbolStr> coerceSymbolStrs(
        nix::ref<nix::EvalState>        state
, const std::vector<std::string_view> & lst
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


/* -------------------------------------------------------------------------- */

template <typename T, template <typename, typename> class C>
  static inline bool
hasElement( const C<T, std::allocator<T>> & container, const T & e )
{
  return std::find( container.cbegin(), container.cend(), e ) !=
         container.cend();
}


/* -------------------------------------------------------------------------- */

  static inline bool
shouldSearchSystem( std::string_view system )
{
  return std::find( defaultSystems.cbegin(), defaultSystems.cend(), system ) !=
         defaultSystems.cend();
}


/* -------------------------------------------------------------------------- */

std::list<Resolved> mergeResolvedByAttrPathGlob(
  const std::list<Resolved> & all
);


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
