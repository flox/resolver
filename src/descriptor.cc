/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <variant>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "resolve.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  std::optional<bool>
isAbsAttrPath( const nlohmann::json & j )
{
  if ( ! j.is_array() )
    {
      throw DescriptorException(
        "Descriptor `path' field must be lists of strings or null."
      );
    }

  std::vector<nlohmann::json> path = j;
  if ( path.empty() )
    {
      return std::nullopt;
    }

  if ( path[0].is_null() )
    {
      throw DescriptorException(
        "Descriptor `path' field may only contain `null' as its second member."
      );
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
  bool explicitFlake = false;
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
              this->absAttrPath = std::vector<attr_part>();
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
              this->relAttrPath = value;
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
          this->version = value;
        }
      else if ( key == "semver" )
        {
          this->semver = value;
        }
      else if ( key == "catalog" )
        {
          if ( value.is_boolean() )
            {
              this->searchCatalogs = value;
            }
          else if ( value.is_object() )
            {
              this->searchCatalogs = true;
              if ( ! explicitFlake )
                {
                  this->searchFlakes = false;
                }
              for ( auto & [ckey, cvalue] : value.items() )
                {
                  if ( ckey == "stability" )
                    {
                      this->catalogStability = cvalue;
                    }
                }
            }
          else
            {
              throw DescriptorException(
                "Descriptor `catalog' field must be a string or boolean."
              );
            }
        }
      else if ( key == "flake" )
        {
          this->searchFlakes = value;
          explicitFlake      = true;
        }
    }

  /* Audit fields. */
  std::string msg;
  if ( ! this->audit( msg ) )
    {
      throw DescriptorException( msg );
    }
}


/* -------------------------------------------------------------------------- */

  nlohmann::json
Descriptor::toJSON() const
{
  /* Audit fields. */
  std::string msg;
  if ( ! this->audit( msg ) )
    {
      throw DescriptorException( msg );
    }

  nlohmann::json j = nlohmann::json::object();
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
      j.emplace( "catalog", c );
    }
  else
    {
      j.emplace( "catalog", this->searchCatalogs );
    }

  j.emplace( "flake", this->searchFlakes );

  if ( this->inputId.has_value() )
    {
      j.emplace( "input", this->inputId.value() );
    }

  return j;
}


/* -------------------------------------------------------------------------- */

/* TODO */
  std::string
Descriptor::toString() const
{
  return "";
}


/* -------------------------------------------------------------------------- */

// TODO: Handle case where none of `path' or `name' are set.
// Use `default' or `defaultPackage'.

  static inline bool
auditAbsAttrPath( const Descriptor & d, std::string & msg )
{
  if ( d.absAttrPath.has_value() && d.relAttrPath.has_value() )
    {
      msg = "Descriptor fields `absAttrPath' and `relAttrPath' are "
            "mutually exclusive.";
      return false;
    }

  if ( ! d.absAttrPath.has_value() )
    {
      return true;
    }
  /* Must be at least length 3. */
  if ( d.absAttrPath.value().size() < 3 )
    {
      msg = "Descriptor field `absAttrPath' must contain at least 3 elements.";
      return false;
    }
  /* `nullptr' may only appear as second element. */
  for ( size_t i = 0; i < d.absAttrPath.value().size(); ++i )
    {
      /* Don't audit second element. */
      if ( i == 1 )
        {
          continue;
        }
      if ( std::holds_alternative<std::nullptr_t>( d.absAttrPath.value()[i] ) )
        {
          msg = "Descriptor field `absAttrPath' may only contain `nullptr' as "
                "its second element.";
          return false;
        }
    }
  return true;
}


  static inline bool
auditSemverVersion( const Descriptor & d, std::string & msg )
{
  if ( d.semver.has_value() && d.version.has_value() )
    {
      msg = "Descriptor fields `semver' and `version' are mutually exclusive.";
      return false;
    }
  return true;
}


  static inline bool
auditFlakeCatalog( const Descriptor & d, std::string & msg )
{
  if ( ! ( d.searchFlakes || d.searchCatalogs ) )
    {
      msg = "Descriptors must not disable searching in `flakes' "
            "and `catalogs'.";
      return false;
    }
  if ( d.catalogStability.has_value() && d.searchFlakes )
    {
      msg = "Descriptors which indicate `catalog.stability' must not be "
            "allowed to search in flakes.";
      return false;
    }
  if ( d.catalogStability.has_value() && ( ! d.searchCatalogs ) )
    {
      msg = "Descriptors which indicate `catalog.stability' must be "
            "allowed to search in catalogs.";
      return false;
    }
  return true;
}


  bool
Descriptor::audit( std::string & msg ) const
{
  bool rsl = true;
  msg      = "OK";

  rsl &= auditAbsAttrPath( * this, msg );
  rsl &= auditSemverVersion( * this, msg );
  rsl &= auditFlakeCatalog( * this, msg );

  return rsl;
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

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
