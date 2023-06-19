/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <algorithm>
#include <nix/nixexpr.hh>
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  FloxFlakeRef
coerceFlakeRef( std::string_view uri )
{
  return nix::parseFlakeRef( std::string( uri ) );
}


/* -------------------------------------------------------------------------- */

  std::list<Resolved>
mergeResolvedByAttrPathGlob( const std::list<Resolved> & all )
{
  std::unordered_map<AttrPathGlob, Resolved> bps;
  for ( auto & r : all )
    {
      AttrPathGlob gp( r.path );
      gp.coerceGlob();
      if ( auto search = bps.find( gp ); search != bps.end() )
        {
          for ( auto & [system, sysInfo] : r.info.items() )
            {
              search->second.info.emplace( system, sysInfo );
            }
        }
      else
        {
          Resolved gr( r );
          gr.path.coerceGlob();
          bps.emplace( std::move( gp ), std::move( gr ) );
        }
    }
  std::list<Resolved> rsl;
  for ( auto & [gp, gr] : bps ) { rsl.push_back( std::move( gr ) ); }
  return rsl;
}


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

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
