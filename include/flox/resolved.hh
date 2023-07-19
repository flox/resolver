/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include "flox/types.hh"
#include "flox/flox-flake.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */


class Resolved {
  private:
    FloxFlakeRef _input;

  public:
    std::string    inputId;
    AttrPathGlob   path;
    nlohmann::json info;

    Resolved( const nlohmann::json & attrs )
      : inputId( attrs.at( "input" ).at( "id" ) )
      , _input( nix::FlakeRef::fromAttrs( nix::fetchers::jsonToAttrs(
                  attrs.at( "input" ).at( "locked" )
              ) ) )
      , path( AttrPathGlob::fromJSON( attrs.at( "path" ) ) )
      , info( attrs.at( "info" ) )
    {}

    Resolved(       std::string_view   inputId
            , const FloxFlakeRef     & input
            , const AttrPathGlob     & path
            , const nlohmann::json   & info
            )
      : inputId( inputId ), _input( input ), path( path ), info( info )
    {}

      std::string
    toString() const
    {
      return this->_input.to_string() + "#" + this->path.toString();
    }

      nlohmann::json
    toJSON() const
    {
      return {
        { "input", {
            { "id",     this->inputId }
          , { "locked", nix::fetchers::attrsToJSON( this->_input.toAttrs() ) }
          }
        }
      , { "uri",   this->toString() }
      , { "info",  this->info }
      , { "path",  this->path.toJSON() }
      };
    }
};


void from_json( const nlohmann::json & j,       Resolved & p );
void to_json(         nlohmann::json & j, const Resolved & p );


/* -------------------------------------------------------------------------- */

std::list<Resolved> & mergeResolvedByAttrPathGlob( std::list<Resolved> & lst );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
