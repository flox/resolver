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

AttrPathGlob::AttrPathGlob( const attr_parts & pp )
{
  size_t s = pp.size();
  for ( size_t i = 0; i < s; ++i )
    {
      if ( ( std::holds_alternative<std::nullptr_t>( pp[i] ) ) ||
           ( std::get<std::string>( pp[i] ) == "{{system}}" )
         )
        {
          if ( i != 1 )
            {
              throw ResolverException(
                "Resolved `path' may only contain `null' as its second member."
              );
            }
          this->path.emplace_back( nullptr );
        }
      else
        {
          this->path.emplace_back( pp[i] );
        }
    }
}


/* -------------------------------------------------------------------------- */

AttrPathGlob::AttrPathGlob( attr_parts && pp )
{
  size_t s = pp.size();
  bool nsys = false;
  for ( size_t i = 0; i < s; ++i )
    {
      if ( ( std::holds_alternative<std::nullptr_t>( pp[i] ) ) ||
           ( std::get<std::string>( pp[i] ) == "{{system}}" )
         )
        {
          if ( i == 1 )
            {
              nsys = true;
            }
          else
            {
              throw ResolverException(
                "Resolved `path' may only contain `null' as its second member."
              );
            }
        }
    }
  this->path = std::move( pp );
  if ( nsys ) { this->path[1] = nullptr; }
}


/* -------------------------------------------------------------------------- */

  AttrPathGlob
AttrPathGlob::fromStrings( const std::vector<std::string> & pp )
{
  AttrPathGlob ap;
  for ( auto & p : pp )
    {
      if ( p == "{{system}}" ) { ap.path.push_back( nullptr ); }
      else                     { ap.path.push_back( p ); }
    }
  return ap;
}

  AttrPathGlob
AttrPathGlob::fromStrings( const std::vector<std::string_view> & pp )
{
  AttrPathGlob ap;
  for ( auto & p : pp )
    {
      if ( p == "{{system}}" ) { ap.path.push_back( nullptr ); }
      else                     { ap.path.push_back( std::string( p ) ); }
    }
  return ap;
}


/* -------------------------------------------------------------------------- */

  AttrPathGlob
AttrPathGlob::fromJSON( const nlohmann::json & pp )
{
  if ( ! pp.is_array() )
    {
      throw ResolverException(
        "AttrPathGlobs must be constructed using an array, but argument "
        "is of type '" + std::string( pp.type_name() ) + "'"
      );
    }
  AttrPathGlob ap;
  for ( auto & p : pp )
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
      this->path[1] = nullptr;
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
          str += "{{system}}";
        }
      else
        {
          if ( std::get<std::string>( this->path[i] ).find( '.' ) !=
               std::string::npos
             )
            {
              str += "\"" + std::get<std::string>( this->path[i] ) + "\"";
            }
          str += std::get<std::string>( this->path[i] );
        }
      if ( ( i + 1 ) < this->path.size() ) { str += "."; }
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
