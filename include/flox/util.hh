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
#include "flox/types.hh"
#include <optional>
#include <vector>


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

static nix::flake::LockFlags floxFlakeLockFlags = {
  .updateLockFile = false
, .writeLockFile  = false
};


/* -------------------------------------------------------------------------- */

FloxFlakeRef coerceFlakeRef( std::string_view uri );
FloxFlakeRef coerceFlakeRef( FloxFlakeRef & ref );


/* -------------------------------------------------------------------------- */

std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
  nix::EvalState   & state
, std::string_view   uri
);

std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
        nix::EvalState & state
, const FloxFlakeRef   & ref
);

std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
  nix::EvalState                           & state
, std::shared_ptr<nix::flake::LockedFlake> & locked
);


/* -------------------------------------------------------------------------- */

nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
  nix::EvalState   & state
, std::string_view   uri
);

nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
        nix::EvalState & state
, const FloxFlakeRef   & ref
);

nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
  nix::EvalState                           & state
, std::shared_ptr<nix::flake::LockedFlake> & locked
);

nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
  nix::EvalState                       & state
, nix::ref<nix::eval_cache::EvalCache> & cache
);


/* -------------------------------------------------------------------------- */

std::vector<nix::Symbol> coerceSymbols(
        nix::EvalState                & state
, const std::vector<std::string_view> & lst
);

std::vector<nix::Symbol> coerceSymbols(
  nix::EvalState           & state
, std::vector<nix::Symbol> & lst
);


/* -------------------------------------------------------------------------- */

std::vector<nix::SymbolStr> coerceSymbolStrs(
        nix::EvalState                & state
, const std::vector<std::string_view> & lst
);

std::vector<nix::SymbolStr> coerceSymbolStrs(
        nix::EvalState           & state
, const std::vector<nix::Symbol> & lst
);

std::vector<nix::SymbolStr> coerceSymbolStrs(
  nix::EvalState              & state
, std::vector<nix::SymbolStr> & lst
);


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
