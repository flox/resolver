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
  , path( attrs.at( "path" ) )
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

  std::vector<Resolved>
resolve( const Inputs      & inputs
       , const Preferences & preferences
       , const Descriptor  & desc
       )
{
  std::vector<Resolved> rsl;
  nix::initNix();
  nix::initGC();

  // TODO: Add a flag
  nix::evalSettings.pureEval = false;

  nix::EvalState state( {}, nix::openStore() );

  // TODO: Add flags
  nix::flake::LockFlags lockFlags = {
    .updateLockFile = false
  , .writeLockFile  = false
  };

    std::unordered_map<std::string, std::shared_ptr<nix::flake::LockedFlake>>
  lockedInputs;

  for ( auto & [id, ref] : inputs.inputs )
    {
      auto flake = std::make_shared<nix::flake::LockedFlake>(
        nix::flake::lockFlake( state, ref, lockFlags )
      );
      lockedInputs.emplace( id, flake );

      // TODO: write helper
      //auto cache = nix::openEvalCache( state, flake );

      // TODO

    }

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
