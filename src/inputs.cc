/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <nlohmann/json.hpp>
#include <nix/shared.hh>
#include <nix/eval.hh>
#include <nix/eval-inline.hh>
#include <nix/flake/flake.hh>
#include <nix/store-api.hh>
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

  void
Inputs::lockOne( std::string_view id )
{

}


/* -------------------------------------------------------------------------- */

  void
Inputs::lockAll()
{

}


/* -------------------------------------------------------------------------- */

  bool
Inputs::has( std::string_view id ) const
{
  return false; // TODO
}


//  FloxFlakeInput
//Inputs::get( std::string_view id ) const
//{
//
//}


//  FloxFlakeInput
//Inputs::getLocked( std::string_view id ) const
//{
//
//}


/* -------------------------------------------------------------------------- */

// TODO
  nlohmann::json
Inputs::toJSON() const
{
  nlohmann::json j;
  return j;
}


/* -------------------------------------------------------------------------- */

// TODO
  nlohmann::json
Inputs::lockedToJSON()
{
  nlohmann::json j;
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
