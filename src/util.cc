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

  std::list<Resolved>
mergeResolvedByAttrPathGlob( const std::list<Resolved> & all )
{
  std::unordered_map<AttrPathGlob, Resolved> bps;
  for ( auto & r : all )
    {
      AttrPathGlob gp( r.path );
      gp.coerceGlob();
      if ( auto search = bps.find( gp ); search != bps.end() )
        {
          for ( auto & [system, sysInfo] : r.info.items() )
            {
              search->second.info.emplace( system, sysInfo );
            }
        }
      else
        {
          Resolved gr( r );
          gr.path.coerceGlob();
          bps.emplace( std::move( gp ), std::move( gr ) );
        }
    }
  std::list<Resolved> rsl;
  for ( auto & [gp, gr] : bps ) { rsl.push_back( std::move( gr ) ); }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::list<Resolved> &
mergeResolvedByAttrPathGlob( std::list<Resolved> & lst )
{
  std::unordered_map<AttrPathGlob, Resolved> bps;
  while ( ! lst.empty() )
    {
      lst.front().path.coerceGlob();
      if ( auto search = bps.find( lst.front().path ); search != bps.end() )
        {
          for ( auto & [system, sysInfo] : lst.front().info.items() )
            {
              search->second.info.emplace( system, std::move( sysInfo ) );
            }
        }
      else
        {
          lst.front().path.coerceGlob();
          AttrPathGlob gp = lst.front().path;
          bps.emplace( std::move( gp ), std::move( lst.front() ) );
        }
      lst.pop_front();
    }
  for ( auto & [gp, gr] : bps ) { lst.push_back( std::move( gr ) ); }
  return lst;
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
