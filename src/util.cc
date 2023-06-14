/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <algorithm>
#include <nix/nixexpr.hh>
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


/* -------------------------------------------------------------------------- */

std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
        nix::ref<nix::EvalState>   state
, const FloxFlakeRef             & ref
)
{
  return std::make_shared<nix::flake::LockedFlake>( nix::flake::LockedFlake(
    nix::flake::lockFlake( * state, ref, floxFlakeLockFlags )
  ) );
}

std::shared_ptr<nix::flake::LockedFlake> coerceLockedFlake(
  nix::ref<nix::EvalState> state
, std::string_view         uri
)
{
  return coerceLockedFlake( state, coerceFlakeRef( uri ) );
}


/* -------------------------------------------------------------------------- */

nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
  nix::ref<nix::EvalState> state
, std::string_view         uri
)
{
  std::shared_ptr<nix::flake::LockedFlake> locked =
    coerceLockedFlake( state, uri );
  return coerceEvalCache( state, locked );
}

nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
        nix::ref<nix::EvalState>   state
, const FloxFlakeRef             & ref
)
{
  std::shared_ptr<nix::flake::LockedFlake> locked =
    coerceLockedFlake( state, ref );
  return coerceEvalCache( state, locked );
}

nix::ref<nix::eval_cache::EvalCache> coerceEvalCache(
  nix::ref<nix::EvalState>                   state
, std::shared_ptr<nix::flake::LockedFlake> & locked
)
{
  return nix::openEvalCache( * state, locked );
}


/* -------------------------------------------------------------------------- */

std::vector<nix::Symbol> coerceSymbols(
        nix::ref<nix::EvalState>        state
, const std::vector<std::string_view> & lst
)
{
  std::vector<nix::Symbol> rsl;
  for ( auto & e : lst ) { rsl.push_back( state->symbols.create( e ) ); }
  return rsl;
}


/* -------------------------------------------------------------------------- */

std::vector<nix::SymbolStr> coerceSymbolStrs(
        nix::ref<nix::EvalState>        state
, const std::vector<std::string_view> & lst
)
{
  return state->symbols.resolve( coerceSymbols( state, lst ) );
}


/* -------------------------------------------------------------------------- */

  std::map<std::string, std::shared_ptr<nix::flake::LockedFlake>>
prepInputs(       nix::ref<nix::EvalState>   state
          , const Inputs                   & inputs
          , const Preferences              & prefs
          )
{
  /* Set impure while we lock inputs. */
  bool oldPure = nix::evalSettings.pureEval;
  nix::evalSettings.pureEval = false;

  std::vector<input_pair> ins;
  for ( auto & [id, ref] : inputs.inputs )
    {
      std::shared_ptr<nix::flake::LockedFlake> locked =
        coerceLockedFlake( state, ref );
      ins.push_back( std::make_pair( id, locked ) );
    }
  std::sort( ins.begin(), ins.end(), prefs.inputLessThan() );

  std::map<std::string, std::shared_ptr<nix::flake::LockedFlake>> prep;
  for ( auto & i : ins )
    {
      prep.emplace( i.first, i.second );
    }

  /* Restore initial `pureEval' setting. */
  nix::evalSettings.pureEval = oldPure;
  return prep;
}


/* -------------------------------------------------------------------------- */

  std::vector<CursorPos>
globSystems(       nix::ref<nix::EvalState>   state
           ,       CursorPos                & c
           , const std::list<std::string>   & systems
           )
{
  std::vector<CursorPos> rsl;
  for ( auto & s : systems )
    {
      nix::Symbol ss = state->symbols.create( s );
      MaybeCursor mc = c.first->maybeGetAttr( ss );
      if ( mc != nullptr )
        {
          std::vector<nix::Symbol> path( c.second );
          path.push_back( ss );
          CursorPos r = std::make_pair( Cursor( mc ), path );
          rsl.push_back( r );
        }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  bool
sortByDepth( const AttrPathGlob & a, const AttrPathGlob & b ) noexcept
{
  return a.path.size() <= b.path.size();
}


/* -------------------------------------------------------------------------- */

  nix::Value *
loadFlakeRoot(
  nix::ref<nix::EvalState>                 state
, std::shared_ptr<nix::flake::LockedFlake> lockedFlake
)
{
  nix::Value * vFlake = state->allocValue();
  nix::flake::callFlake( * state, * lockedFlake, * vFlake );
  state->forceAttrs( * vFlake, nix::noPos, "while parsing cached flake data" );
  nix::Attr * aOutputs = vFlake->attrs->get(
    state->symbols.create( "outputs" )
  );
  assert( aOutputs != nullptr );
  return aOutputs->value;
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
