/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <iostream>

#include <string>
#include <functional>
#include <vector>
#include "flox/predicates.hh"
#include "flox/package.hh"
#include "semver.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {
    namespace predicates {

/* -------------------------------------------------------------------------- */

  PkgPred
hasName( const std::string & name )
{
  return (PkgPred::pred_fn) [name]( const Package & p )
  {
    return ( p.getPkgAttrName() == name ) ||
           ( p.getPname() == name )  ||
           ( p.getFullName() == name );
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
hasFullName( const std::string & name )
{
  return (PkgPred::pred_fn) [name]( const Package & p )
  {
    return p.getFullName() == name;
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
hasPname( const std::string & name )
{
  return (PkgPred::pred_fn) [name]( const Package & p )
  {
    return p.getPname() == name;
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
hasPkgAttrName( const std::string & name )
{
  return (PkgPred::pred_fn) [name]( const Package & p )
  {
    return p.getPkgAttrName() == name;
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
hasVersion( const std::string & version )
{
  return (PkgPred::pred_fn) [version]( const Package & p )
  {
    return p.getVersion().has_value() && ( p.getVersion().value() == version );
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
satisfiesSemver( const std::string & range )
{
  return (PkgPred::pred_fn) [range]( const Package & p )
  {
    if ( ! p.getSemver().has_value() ) { return false; }
    std::list<std::string> vs;
    vs.push_back( p.getSemver().value() );
    return ! semverSat( range, vs ).empty();
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
hasLicense( const std::string & license )
{
  return (PkgPred::pred_fn) [license]( const Package & p )
  {
    return p.getLicense().has_value() && ( p.getLicense().value() == license );
  };
}

  PkgPred
hasLicense( const std::vector<std::string> & licenses )
{
  return (PkgPred::pred_fn) [licenses]( const Package & p )
  {
    return
      p.getLicense().has_value() &&
      ( std::find( licenses.begin(), licenses.end(), p.getLicense().value() ) !=
        licenses.end()
      );
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
hasSubtree( const std::string & subtree )
{
  return (PkgPred::pred_fn) [subtree]( const Package & p )
  {
    switch ( p.getSubtreeType() )
      {
        case ST_PACKAGES: return subtree == "packages";       break;
        case ST_LEGACY:   return subtree == "legacyPackages"; break;
        case ST_CATALOG:  return subtree == "catalog";        break;
        default:          return false;                       break;
      }
    return false;
  };
}

  PkgPred
hasSubtree( subtree_type subtree )
{
  return (PkgPred::pred_fn) [subtree]( const Package & p )
  {
    return p.getSubtreeType() == subtree;
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
hasAbsPathPrefix( const std::vector<std::string> & prefix )
{
  return (PkgPred::pred_fn) [prefix]( const Package & p )
  {
    const std::vector<std::string> path = p.getPathStrs();
    if ( path.size() < prefix.size() ) { return false; }
    for ( size_t i = 0; i < prefix.size(); ++i )
      {
        if ( prefix[i] != path[i] ) { return false; }
      }
    return true;
  };
}

  PkgPred
hasAbsPathPrefix( const AttrPathGlob & prefix )
{
  std::vector<std::string> sprefix;
  if ( ! prefix.hasGlob() )
    {
      for ( const attr_part & p : prefix.path )
        {
          sprefix.push_back( std::get<std::string>( p ) );
        }
      return hasAbsPathPrefix( sprefix );
    }
  for ( size_t i = 0; i < prefix.path.size(); ++i )
    {
      if ( i == 2 ) { continue; }
      sprefix.push_back( std::get<std::string>( prefix.path[i] ) );
    }
  return (PkgPred::pred_fn) [sprefix]( const Package & p )
  {
    const std::vector<std::string> path = p.getPathStrs();
    if ( ( path.size() - 1) < sprefix.size() ) { return false; }
    for ( size_t i = 0; i < sprefix.size(); ++i )
      {
        if ( i == 2 )                { continue; }
        if ( sprefix[i] != path[i] ) { return false; }
      }
    return true;
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
hasRelPathPrefix( const std::vector<std::string> & prefix )
{
  return (PkgPred::pred_fn) [prefix]( const Package & p )
  {
    const std::vector<std::string> path = p.getPathStrs();
    if ( ( path.size() - 2 ) < prefix.size() ) { return false; }
    for ( size_t i = 0; i < prefix.size(); ++i )
      {
        if ( prefix[i] != path[i + 2] ) { return false; }
      }
    return true;
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
hasUnfree( bool value )
{
  return (PkgPred::pred_fn) [value]( const Package & p )
  {
    return p.isUnfree().has_value() && ( p.isUnfree().value() == value );
  };
}


/* -------------------------------------------------------------------------- */


  PkgPred
hasBroken( bool value )
{
  return (PkgPred::pred_fn) [value]( const Package & p )
  {
    return p.isBroken().has_value() && ( p.isBroken().value() == value );
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
hasOutput( const std::string & output )
{
  return (PkgPred::pred_fn) [output]( const Package & p )
  {
    const std::vector<std::string> outputs = p.getOutputs();
    return std::find( outputs.cbegin(), outputs.cend(), output ) !=
           outputs.cend();
  };
}

  PkgPred
hasOutputs( const std::vector<std::string> & outputs )
{
  return (PkgPred::pred_fn) [outputs]( const Package & p )
  {
    const std::vector<std::string> haves = p.getOutputs();
    for ( const auto & o : outputs )
      {
        if ( std::find( haves.cbegin(), haves.cend(), o ) == haves.cend() )
          {
            return false;
          }
      }
    return true;
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
depthLE( size_t max )
{
  return (PkgPred::pred_fn) [max]( const Package & p )
  {
    return p.getPathStrs().size() <= max;
  };
}


/* -------------------------------------------------------------------------- */

  PkgPred
hasStability( const std::string & stability )
{
  return (PkgPred::pred_fn) [stability]( const Package & p )
  {
    return p.getStability().has_value() &&
           ( p.getStability().value() == stability );
  };
}


/* -------------------------------------------------------------------------- */

    }  /* End Namespace `flox::resolve::predicates' */
  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
