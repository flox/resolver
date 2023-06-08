/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  FloxFlakeRef
coerceFlakeRef( std::string_view uri )
{
  return nix::parseFlakeRef( std::string( uri ) );
}


FloxFlakeRef coerceFlakeRef( FloxFlakeRef & ref ) { return ref; }


/* -------------------------------------------------------------------------- */

std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
  nix::EvalState   & state
, std::string_view   uri
)
{
  return coerceLockedFlake( state, coerceFlakeRef( uri ) );
}


std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
        nix::EvalState & state
, const FloxFlakeRef   & ref
)
{
  return std::make_shared<nix::flake::LockedFlake>( nix::flake::LockedFlake(
    nix::flake::lockFlake( state, ref, floxFlakeLockFlags )
  ) );
}


std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
  nix::EvalState                           & state
, std::shared_ptr<nix::flake::LockedFlake> & locked
)
{
  return locked;
}


/* -------------------------------------------------------------------------- */

nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
  nix::EvalState   & state
, std::string_view   uri
)
{
  std::shared_ptr<nix::flake::LockedFlake> locked =
    coerceLockedFlake( state, uri );
  return coerceEvalCache( state, locked );
}


nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
        nix::EvalState & state
, const FloxFlakeRef   & ref
)
{
  std::shared_ptr<nix::flake::LockedFlake> locked =
    coerceLockedFlake( state, ref );
  return coerceEvalCache( state, locked );
}


nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
  nix::EvalState                           & state
, std::shared_ptr<nix::flake::LockedFlake> & locked
)
{
  return nix::openEvalCache( state, locked );
}


nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
  nix::EvalState                       & state
, nix::ref<nix::eval_cache::EvalCache> & cache
)
{
  return cache;
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
