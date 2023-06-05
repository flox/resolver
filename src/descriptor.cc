/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <variant>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "descriptor.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  std::optional<bool>
isAbsAttrPath( const nlohmann::json & j )
{
  if ( ! j.is_array() )
    {
      throw "AttrPaths must be lists of strings or null.";
    }

  std::vector<nlohmann::json> path = j;
  if ( path.empty() )
    {
      return std::nullopt;
    }

  if ( path[0].is_null() )
    {
      throw "AttrPaths may only contain `null' as their second member.";
    }
  std::string_view first = path[0].get<std::string_view>();

  return ( first == "packages" ) ||
         ( first == "legacyPackages" ) ||
         ( first == "catalog" );
}


/* -------------------------------------------------------------------------- */

Descriptor::Descriptor( std::string_view desc )
{
  // TODO
}


/* -------------------------------------------------------------------------- */

Descriptor::Descriptor( const nlohmann::json & desc )
{
  bool canSearchAny = true;
  for ( auto & [key, value] : desc.items() )
    {
      if ( key == "path" )
        {
          if ( value.empty() )
            {
              continue;
            }
          if ( isAbsAttrPath( value ) )
            {
              if ( value.size() < 3 )
                {
                  throw "Absolute AttrPaths must be at least 3 elements long.";
                }
              this->absAttrPath = std::vector<attr_part>( value.size() );
              for ( auto & p : value )
                {
                  if ( p.is_null() )
                    {
                      this->absAttrPath.value().push_back( nullptr );
                    }
                  else
                    {
                      this->absAttrPath.value().push_back( p );
                    }
                }
            }
          else
            {
              this->relAttrPath = std::vector<std::string>( value.size() );
              for ( auto & p : value )
                {
                  this->relAttrPath.value().push_back( p );
                }
            }
        }
      else if ( key == "input" )
        {
          this->inputId = value;
        }
      else if ( key == "name" )
        {
          this->name = value;
        }
      else if ( key == "version" )
        {
          if ( this->semver.has_value() )
            {
              throw ( "Descriptor fields `semver' and `version' are "
                      "mutually exclusive." );
            }
          this->version = value;
        }
      else if ( key == "semver" )
        {
          if ( this->version.has_value() )
            {
              throw ( "Descriptor fields `semver' and `version' are "
                      "mutually exclusive." );
            }
          this->semver = value;
        }
      else if ( key == "catalog" )
        {
          if ( value.is_boolean() )
            {
              if ( value && ( ! canSearchAny ) )
                {
                  throw ( "Descriptor field for `catalog' conflicts with "
                          "previous `flake' field." );
                }
              this->searchCatalogs = value;
            }
          else if ( value.is_string() )
            {
              if ( ! canSearchAny )
                {
                  throw ( "Descriptor field for `catalog' conflicts with "
                          "previous `flake' field." );
                }
              if ( this->inputId.has_value() )
                {
                  throw
                    "Descriptors may not set `inputId' related fields twice";
                }
              canSearchAny         = false;
              this->searchCatalogs = true;
              this->searchFlakes   = false;
              this->inputId        = value;
            }
          else if ( value.is_object() )
            {
              if ( ! canSearchAny )
                {
                  throw ( "Descriptor field for `catalog' conflicts with "
                          "previous `flake' field." );
                }
              canSearchAny         = false;
              this->searchCatalogs = true;
              this->searchFlakes   = false;
              for ( auto & [ckey, cvalue] : value.items() )
                {
                  if ( ckey == "id" )
                    {
                      if ( this->inputId.has_value() )
                        {
                          throw ( "Descriptors may not set `inputId' related "
                                  "fields twice" );
                        }
                      this->inputId = cvalue;
                    }
                  else if ( ckey == "stability" )
                    {
                      this->catalogStability = cvalue;
                    }
                }
            }
          else
            {
              throw "Catalog field must be a string, boolean, or attr-set.";
            }
        }
      else if ( key == "flake" )
        {
          if ( value.is_boolean() )
            {
              if ( value && ( ! canSearchAny ) )
                {
                  throw ( "Descriptor field for `flake' conflicts with "
                          "previous `catalog' field." );
                }
              this->searchFlakes = value;
            }
          else if ( value.is_string() )
            {
              if ( ! canSearchAny )
                {
                  throw ( "Descriptor field for `flake' conflicts with "
                          "previous `catalog' field." );
                }
              if ( this->inputId.has_value() )
                {
                  throw
                    "Descriptors may not set `inputId' related fields twice";
                }
              canSearchAny         = false;
              this->searchFlakes   = true;
              this->searchCatalogs = false;
              this->inputId        = value;
            }
          else if ( value.is_object() )
            {
              if ( ! canSearchAny )
                {
                  throw ( "Descriptor field for `flake' conflicts with "
                          "previous `catalog' field." );
                }
              canSearchAny         = false;
              this->searchFlakes   = true;
              this->searchCatalogs = false;
              for ( auto & [fkey, fvalue] : value.items() )
                {
                  if ( fkey == "id" )
                    {
                      if ( this->inputId.has_value() )
                        {
                          throw ( "Descriptors may not set `inputId' related "
                                  "fields twice" );
                        }
                      this->inputId = fvalue;
                    }
                }
            }
          else
            {
              throw "Flake field must be a string, boolean, or attr-set.";
            }
        }
    }
  if ( ! ( this->searchCatalogs || this->searchFlakes ) )
    {
      throw "Descriptor must be able to search in either flakes or catalogs";
    }
}


/* -------------------------------------------------------------------------- */

  nlohmann::json
Descriptor::toJSON() const
{
  nlohmann::json j = nlohmann::json::object();
  if ( this->relAttrPath.has_value() && this->absAttrPath.has_value() )
    {
      throw ( "Descriptor fields `absAttrPath' and `relAttrPath' are "
              "mutually exclusive." );
    }
  if ( this->relAttrPath.has_value() )
    {
      j.emplace( "path", this->relAttrPath.value() );
    }
  else if ( this->absAttrPath.has_value() )
    {
      nlohmann::json path;
      for ( auto & p : this->absAttrPath.value() )
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
      j.emplace( "path", path );
    }

  if ( this->name.has_value() )
    {
      j.emplace( "name", this->name.value() );
    }

  /* These fields are mutually exclusive */
  if ( this->version.has_value() )
    {
      j.emplace( "version", this->version.value() );
    }
  else if ( this->semver.has_value() )
    {
      j.emplace( "semver", this->semver.value() );
    }

  if ( this->catalogStability.has_value() )
    {
      nlohmann::json c = nlohmann::json::object();
      c.emplace( "stability", this->catalogStability.value() );
      if ( ( this->inputId.has_value() ) && ( ! this->searchFlakes ) )
        {
          j.emplace( "id", this->inputId.value() );
        }
      j.emplace( "catalog", c );
    }
  else if ( this->inputId.has_value() && ( ! this->searchFlakes ) )
    {
      nlohmann::json c = nlohmann::json( { { "id", this->inputId.value() } } );
      j.emplace( "catalog", c );
    }
  else
    {
      j.emplace( "catalog", this->searchCatalogs );
    }

  if ( this->inputId.has_value() && ( ! this->searchCatalogs ) )
    {
      nlohmann::json f = nlohmann::json( { { "id", this->inputId.value() } } );
      j.emplace( "flake", f );
    }
  else
    {
      j.emplace( "flake", this->searchFlakes );
    }

  if ( this->inputId.has_value() && this->searchCatalogs && this->searchFlakes )
    {
      j.emplace( "input", this->inputId.value() );
    }
  return j;
}


/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & j, Descriptor & d )
{
  d = Descriptor( j );
}


  void
to_json( nlohmann::json & j, const Descriptor & d )
{
  j = d.toJSON();
}


/* -------------------------------------------------------------------------- */

  std::string
Descriptor::toString() const
{
  return "";
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
