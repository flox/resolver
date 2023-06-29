/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/db-package-set.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

// TODO: change `DrvDb' to accept `std::list<std::string_view'
  bool
DbPackageSet::hasRelPath( const std::list<std::string_view> & path )
{
  std::vector<std::string> rp;
  if ( this->_stability.has_value() )
    {
      rp.emplace_back( this->_stability.value() );
    }
  for ( auto & p : path ) { rp.emplace_back( p ); }
  return this->_db->hasDrv(
    subtreeTypeToString( this->_subtree )
  , this->_system
  , rp
  ).value_or( false );
}


/* -------------------------------------------------------------------------- */

  std::size_t
DbPackageSet::size()
{
  if ( this->_stability.has_value() )
    {
      return this->_db->countDrvInfosStability( this->_system
                                              , this->_stability.value()
                                              );
    }
  else
    {
      return this->_db->countDrvInfos( subtreeTypeToString( this->_subtree )
                                     , this->_system
                                     );
    }
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
