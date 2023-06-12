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

Resolved::Resolved( const nlohmann::json & attrs )
  : uri( attrs.at( "uri" ) )
  , input( nix::FlakeRef::fromAttrs(
             nix::fetchers::jsonToAttrs( attrs.at( "input" ) )
           ) )
  , info( attrs.at( "info" ) )
  , path( AttrPathGlob::fromJSON( attrs.at( "path" ) ) )
{}


/* -------------------------------------------------------------------------- */

Resolved::Resolved( const FloxFlakeRef   & input
                  , const AttrPathGlob   & path
                  , const nlohmann::json & info
                  )
  : input( input ), path( path ), info( info )
{
  this->uri = this->input.to_string() + "#" + this->path.toString();
}


/* -------------------------------------------------------------------------- */

  nlohmann::json
Resolved::toJSON() const
{
  return {
    { "input", nix::fetchers::attrsToJSON( this->input.toAttrs() ) }
  , { "uri",   this->uri }
  , { "info",  this->info }
  , { "path",  this->path.toJSON() }
  };
}


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




/* -------------------------------------------------------------------------- */

  std::vector<Resolved>
resolve( const Inputs      & inputs
       , const Preferences & preferences
       , const Descriptor  & desc
       )
{
  std::vector<Resolved> rsl;
  nix::initNix();
  nix::initGC();

  nix::ref<nix::EvalState> state( new nix::EvalState( {}, nix::openStore() ) );
  DescriptorFunctor funk( * state, preferences, desc );

  std::map<std::string, std::shared_ptr<nix::flake::LockedFlake>> lockedInputs;

  if ( desc.inputId.has_value() )
    {
      nix::evalSettings.pureEval = false;  /* We re-enable below. */
      std::string id = desc.inputId.value();
      // TODO assert input exists
      std::shared_ptr<nix::flake::LockedFlake> locked =
        coerceLockedFlake( * state, inputs.inputs.at( id ) );
      lockedInputs.emplace( id, locked );
    }
  else
    {
      lockedInputs = prepInputs( state, inputs, preferences );
    }

  /* Past this point our inputs are guaranteed to be locked.
   * We enable pure so that we can leverage the eval cache. */
  nix::evalSettings.pureEval = true;

  std::vector<CursorPos> roots;

  for( auto & [id, flake] : lockedInputs )
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
              roots.push_back( std::move( p ) );
            }
        }
    }

  /* Get a cursor for every system. */
  std::unordered_set<std::string> systems;
  if ( desc.absAttrPath.has_value() &&
       ( ! desc.absAttrPath.value().hasGlob() )
     )
    {
      systems.emplace(
        std::get<std::string>( desc.absAttrPath.value().path[1] )
      );
    }
  else
    {
      systems = defaultSystems;
    }
  std::vector<CursorPos> sysRoots = globSystems( * state, roots, systems );

  // TODO: walk

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
  if ( resolved.empty() )
    {
      return std::nullopt;
    }
  return resolved[0];
}



/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
