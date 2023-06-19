/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/fetchers.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include <nix/command.hh>
#include <string>
#include <nlohmann/json.hpp>
#include "resolve.hh"
#include "flox/util.hh"
#include <functional>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  void
to_json( nlohmann::json & j, const Resolved & r )
{
  j = r.toJSON();
}

  void
from_json( const nlohmann::json & j, Resolved & r )
{
  r = Resolved( j );
}


/* -------------------------------------------------------------------------- */

  std::vector<Resolved>
resolve( const Inputs      & inputs
       , const Preferences & preferences
       , const Descriptor  & desc
       )
{
  std::vector<Resolved> rsl;
  nix::setStackSize( 64 * 1024 * 1024 );
  nix::initNix();
  nix::initGC();

  // TODO: make this an option. It risks making cross-system eval impossible.
  nix::evalSettings.enableImportFromDerivation.setDefault( false );

  nix::ref<nix::EvalState> state( new nix::EvalState( {}, nix::openStore() ) );
  DescriptorFunctor funk( state, preferences, desc );

  std::map<std::string, std::shared_ptr<nix::flake::LockedFlake>> lockedInputs;

  if ( desc.inputId.has_value() )
    {
      nix::evalSettings.pureEval = false;  /* We re-enable below. */
      std::string id = desc.inputId.value();
      // TODO assert input exists
      std::shared_ptr<nix::flake::LockedFlake> locked =
        coerceLockedFlake( state, inputs.inputs.at( id ) );
      lockedInputs.emplace( id, locked );
    }
  else
    {
      lockedInputs = prepInputs( state, inputs, preferences );
    }

  /* Past this point our inputs are guaranteed to be locked.
   * We enable pure so that we can leverage the eval cache. */
  nix::evalSettings.pureEval = true;

  std::vector<std::pair<std::string, CursorPos>> roots;

  for ( auto & [id, flake] : lockedInputs )
    {
      for ( auto & p : funk.getRoots( id, flake ) )
        {
          if ( desc.absAttrPath.has_value() &&
               ( state->symbols[p.second[0]] !=
                 std::get<std::string>( desc.absAttrPath.value().path[0] )
               )
             )
            {
              continue;
            }
          else
            {
              roots.push_back( std::make_pair( id, std::move( p ) ) );
            }
        }
    }

  /* Get a cursor for every system. */
  std::list<std::string> systems;
  if ( desc.absAttrPath.has_value() &&
       ( ! desc.absAttrPath.value().hasGlob() )
     )
    {
      systems.push_back(
        std::get<std::string>( desc.absAttrPath.value().path[1] )
      );
    }
  else
    {
      systems = defaultSystems;
    }
  std::vector<std::pair<std::string, CursorPos>> sysRoots;
  for ( auto & p : roots )
    {
      for ( auto & c : globSystems( state, p.second, systems ) )
        {
          sysRoots.push_back( std::make_pair( p.first, std::move( c ) ) );
        }
    }

  // TODO: handle `(abs|rel)AttrPath'

  auto sortByDepth = []( const AttrPathGlob & a, const AttrPathGlob & b )
  {
    return a.path.size() <= b.path.size();
  };

  auto collect = [&]()
  {
    std::vector<AttrPathGlob> keys;
    for ( auto & [path, r] : funk.results ) { keys.push_back( path ); }
    std::sort( keys.begin(), keys.end(), sortByDepth );
    for ( auto & path : keys )
      {
        rsl.push_back( std::move( funk.results.at( path ) ) );
      }
    funk.results.clear();
  };

  std::string  prevId;
  for ( auto s : sysRoots )
    {
      if ( prevId != s.first )
        {
          if ( ! prevId.empty() ) { collect(); }
          prevId = s.first;
        }
      funk.visit( lockedInputs.at( s.first )->flake.lockedRef
                , s.second.first
                , s.second.second
                );
    }
  collect();

  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::optional<Resolved>
resolveOne( const Inputs      & inputs
          , const Preferences & preferences
          , const Descriptor  & desc
          )
{
  std::vector<Resolved> resolved = resolve( inputs, preferences, desc );
  if ( resolved.empty() ) { return std::nullopt; }
  return resolved[0];
}



/* -------------------------------------------------------------------------- */

  std::list<Resolved>
resolve_V2( ResolverState & rs, const Descriptor    & desc, bool one )
{
  /* See if we can take shortcuts with this descriptor. */
  if ( desc.inputId.has_value() )
    {
      return rs.resolveInInput( desc.inputId.value(), desc );
    }
  std::list<Resolved> results;
  for ( std::string & id : rs.getInputNames() )
    {
      results.splice( results.end(), rs.resolveInInput( id, desc ) );
      if ( one && ( ! results.empty() ) )
        {
          results.erase( ++results.begin(), results.end() );
          return results;
        }
    }
  return results;
}


/* -------------------------------------------------------------------------- */

  std::optional<Resolved>
resolveOne_V2( ResolverState & rs, const Descriptor & desc )
{
  std::list<Resolved> resolved = resolve_V2( rs, desc, true );
  if ( resolved.empty() ) { return std::nullopt; }
  return resolved.front();
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
