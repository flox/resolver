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
{
  for ( auto & p : attrs.at( "path" ) )
    {
      if ( p.is_null() )
        {
          this->path.push_back( nullptr );
        }
      else
        {
          this->path.push_back( p );
        }
    }
}


/* -------------------------------------------------------------------------- */

Resolved::Resolved( const FloxFlakeRef           & input
                  , const std::vector<attr_part> & path
                  , const nlohmann::json         & info
                  )
  : input( input ), path( path ), info( info )
{
  this->uri = this->input.to_string() + "#";
  bool first = true;
  for ( auto & p : this->path )
    {
      if ( std::holds_alternative<std::nullptr_t>( p ) )
        {
          if ( first )
            {
              throw ResolverException(
                "Resolved `path' may only contain `null' as its second member."
              );
            }
          this->uri += ".{{system}}";
        }
      else
        {
          if ( first )
            {
              first = false;
            }
          else
            {
              this->uri += ".";
            }
          this->uri += std::get<std::string>( p );
        }
    }
}


/* -------------------------------------------------------------------------- */

  nlohmann::json
Resolved::toJSON() const
{
  nlohmann::json path;
  for ( auto & p : this->path )
    {
      if ( std::holds_alternative<std::nullptr_t>( p ) )
        {
          path.push_back( nlohmann::json() );
        }
      else
        {
          path.push_back( std::get<std::string>( p ) );
        }
    }
  return {
    { "input", nix::fetchers::attrsToJSON( this->input.toAttrs() ) }
  , { "uri",   this->uri }
  , { "info",  this->info }
  , { "path",  path }
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
  nix::evalSettings.pureEval = false;
  nix::EvalState state( {}, nix::openStore() );

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

      auto cache = nix::openEvalCache( state, flake );

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
