/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <unordered_map>
#include "resolve.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */


  void
Inputs::init( const nlohmann::json & j )
{
  for ( auto & [id, input] : j.items() )
    {
      if ( input.is_string() )
        {
          this->inputs.emplace( id
                              , nix::parseFlakeRef( input.get<std::string>() )
                              );
        }
      else if ( input.is_object() )
        {
          this->inputs.emplace(
            id
          , FloxFlakeRef::fromAttrs( nix::fetchers::jsonToAttrs( input ) )
          );
        }
    }
}


/* -------------------------------------------------------------------------- */

  bool
Inputs::has( std::string_view id ) const
{
  return this->inputs.find( std::string( id ) ) != this->inputs.end();
}


  FloxFlakeRef
Inputs::get( std::string_view id ) const
{
  return this->inputs.at( std::string( id ) );
}


/* -------------------------------------------------------------------------- */

  nlohmann::json
Inputs::toJSON() const
{
  nlohmann::json j;
  for ( auto & [id, input] : this->inputs )
    {
      j.emplace( id, nix::fetchers::attrsToJSON( input.toAttrs() ) );
    }
  return j;
}


/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & j, Inputs & i )
{
  i.init( j );
}


  void
to_json( nlohmann::json & j, const Inputs & i )
{
  j = i.toJSON();
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
