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

AttrPathGlob::AttrPathGlob( std::vector<std::string_view> path )
{
  for ( auto & p : path ) { this->path.push_back( std::string( p ) ); }
}

AttrPathGlob::AttrPathGlob( std::vector<attr_part> path )
{
  for ( auto & p : path )
    {
      if ( std::holds_alternative<std::nullptr_t>( p ) )
        {
          this->path.push_back( nullptr );
        }
      else
        {
          this->path.push_back( p );
        }
    }
}

AttrPathGlob::AttrPathGlob( const nlohmann::json & path )
{
  for ( auto & p : path )
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
       ( ! std::holds_alternative<std::nullptr_t>( this->path[1] ) )
     )
    {
      this->path[1] = nullptr;
    }
}



/* -------------------------------------------------------------------------- */

  std::string
AttrPathGlob::toString() const
{
  std::string str;
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
          str += ".{{system}}";
        }
      else
        {
          if ( first )
            {
              first = false;
            }
          else
            {
              str += ".";
            }
          str += std::get<std::string>( p );
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
