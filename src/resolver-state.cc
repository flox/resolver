/* ========================================================================== *
 *
 * This is largely borrowed from `<nix>/src/libcmd/commands.hh' except we drop
 * `run' member functions, parsers, and some other unnecessary portions.
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
#include "flox/util.hh"
#include <optional>
#include <vector>
#include <map>
#include <memory>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  nix::ref<nix::Store>
ResolverState::getStore()
{
  if ( this->_store == nullptr )
    {
      this->_store = nix::openStore();
    }
  return nix::ref<nix::Store>( this->_store );
}


/* -------------------------------------------------------------------------- */

  nix::ref<nix::Store>
ResolverState::getEvalStore()
{
  if ( this->evalStore == nullptr )
    {
      this->evalStore = this->getStore();
    }
  return nix::ref<nix::Store>( this->evalStore );
}


/* -------------------------------------------------------------------------- */

  nix::ref<nix::EvalState>
ResolverState::getEvalState()
{
  if ( this->evalState == nullptr )
    {
#if HAVE_BOEHMGC
      this->evalState = std::allocate_shared<nix::EvalState>(
        std::traceable_allocator<nix::EvalState>()
      , {}
      , this->getEvalStore()
      , this->getStore()
      );
#else
      this->evalState = std::make_shared<nix::EvalState>(
        {}
      , this->getEvalStore()
      , this->getStore()
      );
#endif
      this->evalState->repair = nix::NoRepair;
    }
  return nix::ref<nix::EvalState>( this->evalState );
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
