/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <string>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
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

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
