/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <string>
#include <nlohmann/json.hpp>
#include "flox/types.hh"
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

AttrPathGlob::AttrPathGlob( const attr_parts & path )
{
  for ( size_t i = 0; i < path.size(); ++i )
    {
      if ( ( std::holds_alternative<std::nullptr_t>( path[i] ) ) ||
           ( std::get<std::string>( path[i] ) == "{{system}}" )
         )
        {
          if ( i != 1 )
            {
              throw ResolverException(
                "Resolved `path' may only contain `null' as its second member."
              );
            }
          this->path.push_back( nullptr );
        }
      else
        {
          this->path.push_back( path[i] );
        }
    }
}

  AttrPathGlob
AttrPathGlob::fromStrings( const std::vector<std::string> & path )
{
  AttrPathGlob ap;
  for ( auto & p : path )
    {
      if ( p == "{{system}}" ) { ap.path.push_back( nullptr ); }
      else                     { ap.path.push_back( p ); }
    }
  return ap;
}

  AttrPathGlob
AttrPathGlob::fromStrings( const std::vector<std::string_view> & path )
{
  AttrPathGlob ap;
  for ( auto & p : path )
    {
      if ( p == "{{system}}" ) { ap.path.push_back( nullptr ); }
      else                     { ap.path.push_back( std::string( p ) ); }
    }
  return ap;
}

  AttrPathGlob
AttrPathGlob::fromJSON( const nlohmann::json & path )
{
  if ( ! path.is_array() )
    {
      throw ResolverException(
        "AttrPathGlobs must be constructed using an array, but argument "
        "is of type '" + std::string( path.type_name() ) + "'"
      );
    }
  AttrPathGlob ap;
  for ( auto & p : path )
    {
      if ( p.is_null() )
        {
          ap.path.push_back( nullptr );
        }
      else
        {
          ap.path.push_back( p );
        }
    }
  return ap;
}


/* -------------------------------------------------------------------------- */

  bool
AttrPathGlob::isAbsolute() const
{
  return ( 0 < this->path.size() ) &&
         isPkgsSubtree( std::get<std::string>( this->path[0] ) );
}


/* -------------------------------------------------------------------------- */

  bool
AttrPathGlob::hasGlob() const
{
  return ( 1 < this->path.size() ) &&
         std::holds_alternative<std::nullptr_t>( this->path[1] );
}


/* -------------------------------------------------------------------------- */

  void
AttrPathGlob::coerceGlob()
{
  if ( ( 1 < this->path.size() ) &&
       ( ! std::holds_alternative<std::nullptr_t>( this->path[1] ) ) &&
       isPkgsSubtree( std::get<std::string>( this->path[0] ) )
     )
    {
      const std::string s = std::get<std::string>( this->path[1] );
      bool found = false;
      for ( const std::string & system : defaultSystems )
        {
          found = true;
          break;
        }
      if ( ! found ) { this->path[1] = nullptr; }
    }
}


/* -------------------------------------------------------------------------- */

  void
AttrPathGlob::coerceRelative()
{
  if ( this->isAbsolute() )
    {
      this->path.erase( this->path.begin(), this->path.begin() + 2 );
    }
}


/* -------------------------------------------------------------------------- */

  bool
AttrPathGlob::globEq( const AttrPathGlob & other ) const
{
  if ( this->path.size() != other.path.size() ) { return false; }
  // Skip `{{system}}' element
  for ( size_t i = 3; i < this->path.size(); ++i )
    {
      if ( this->path[i] != other.path[i] ) { return false; }
    }
  return true;
}


/* -------------------------------------------------------------------------- */

  bool
AttrPathGlob::operator==( const AttrPathGlob & other ) const
{
  return this->path == other.path;
}


/* -------------------------------------------------------------------------- */

  std::string
AttrPathGlob::toString() const
{
  std::string str;
  for ( size_t i = 0; i < this->path.size(); ++i )
    {
      if ( std::holds_alternative<std::nullptr_t>( this->path[i] ) )
        {
          if ( i != 1 )
            {
              throw ResolverException(
                "Resolved `path' may only contain `null' as its second member."
              );
            }
          str += ".{{system}}";
        }
      else
        {
          if ( i != 0 ) { str += "."; }
          str += std::get<std::string>( this->path[i] );
        }
    }
  return str;
}

  nlohmann::json
AttrPathGlob::toJSON() const
{
  nlohmann::json j;
  for ( auto & p : this->path )
    {
      if ( std::holds_alternative<std::nullptr_t>( p ) )
        {
          j.push_back( nlohmann::json() );
        }
      else
        {
          j.push_back( std::get<std::string>( p ) );
        }
    }
  return j;
}


/* -------------------------------------------------------------------------- */

  void
to_json( nlohmann::json & j, const AttrPathGlob & path )
{
  j = path.toJSON();
}

  void
from_json( const nlohmann::json & j, AttrPathGlob & path )
{
  path = AttrPathGlob( j );
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
