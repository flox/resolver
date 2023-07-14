/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <algorithm>
#include <nix/nixexpr.hh>
#include "flox/util.hh"
#include "flox/types.hh"
#include <filesystem>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  std::list<Resolved> &
mergeResolvedByAttrPathGlob( std::list<Resolved> & lst )
{
  std::unordered_map<AttrPathGlob, Resolved> bps;
  while ( ! lst.empty() )
    {
      lst.front().path.coerceGlob();
      if ( auto search = bps.find( lst.front().path ); search != bps.end() )
        {
          for ( auto & [system, sysInfo] : lst.front().info.items() )
            {
              search->second.info.emplace( system, std::move( sysInfo ) );
            }
        }
      else
        {
          lst.front().path.coerceGlob();
          AttrPathGlob gp = lst.front().path;
          bps.emplace( std::move( gp ), std::move( lst.front() ) );
        }
      lst.pop_front();
    }
  for ( auto & [gp, gr] : bps ) { lst.push_back( std::move( gr ) ); }
  return lst;
}


/* -------------------------------------------------------------------------- */

  std::string_view
subtreeTypeToString( const subtree_type & st )
{
  switch ( st )
    {
      case ST_LEGACY:   return "legacyPackages"; break;
      case ST_PACKAGES: return "packages";       break;
      case ST_CATALOG:  return "catalog";        break;
      case ST_NONE:     return "NONE";           break;
      default:
        throw ResolverException( "Failed to identify invalid subtree." );
    }
}


/* -------------------------------------------------------------------------- */

  subtree_type
parseSubtreeType( std::string_view subtree )
{
  if ( subtree == "legacyPackages" ) { return ST_LEGACY;   }
  if ( subtree == "packages" )       { return ST_PACKAGES; }
  if ( subtree == "catalog" )        { return ST_CATALOG;  }
  throw ResolverException(
    "Failed to parse invalid subtree '" + std::string( subtree ) + "'."
  );
}


/* -------------------------------------------------------------------------- */

  bool
isSubstitutable( std::string_view storePath )
{
  for ( const nix::ref<nix::Store> & s : nix::getDefaultSubstituters() )
    {
      if ( s->isValidPath( s->parseStorePath( storePath ) ) ) { return true; }
    }
  return false;
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
