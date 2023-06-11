/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <algorithm>
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

std::vector<nix::Symbol> coerceSymbols(
        nix::EvalState                & state
, const std::vector<std::string_view> & lst
)
{
  std::vector<nix::Symbol> rsl;
  for ( auto & e : lst ) { rsl.push_back( state.symbols.create( e ) ); }
  return rsl;
}

std::vector<nix::Symbol> coerceSymbols(
  nix::EvalState           & state
, std::vector<nix::Symbol> & lst
)
{
  return lst;
}


/* -------------------------------------------------------------------------- */

std::vector<nix::SymbolStr> coerceSymbolStrs(
        nix::EvalState                & state
, const std::vector<std::string_view> & lst
)
{
  return state.symbols.resolve( coerceSymbols( state, lst ) );
}

std::vector<nix::SymbolStr> coerceSymbolStrs(
        nix::EvalState           & state
, const std::vector<nix::Symbol> & lst
)
{
  return state.symbols.resolve( lst );
}

std::vector<nix::SymbolStr> coerceSymbolStrs(
  nix::EvalState              & state
, std::vector<nix::SymbolStr> & lst
)
{
  return lst;
}


/* -------------------------------------------------------------------------- */

  std::map<std::string, std::shared_ptr<nix::flake::LockedFlake>>
prepInputs(       nix::ref<nix::EvalState>   state
          , const Inputs                   & inputs
          , const Preferences              & prefs
          )
{
  std::vector<input_pair> ins;
  for ( auto & [id, ref] : inputs.inputs )
    {
      std::shared_ptr<nix::flake::LockedFlake> locked =
        coerceLockedFlake( * state, ref );
      ins.push_back( std::make_pair( id, locked ) );
    }
  std::sort( ins.begin(), ins.end(), prefs.inputLessThan() );

  std::map<std::string, std::shared_ptr<nix::flake::LockedFlake>> prep;
  for ( auto & i : ins )
    {
      prep.emplace( i.first, i.second );
    }
  return prep;
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
