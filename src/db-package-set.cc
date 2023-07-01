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

  std::shared_ptr<Package>
DbPackageSet::maybeGetRelPath( const std::list<std::string_view> & path )
{
  std::vector<std::string> p;
  if ( this->_stability.has_value() )
    {
      p.emplace_back( this->_stability.value() );
    }
  for ( const auto & s : path ) { p.emplace_back( s ); }
  std::optional<nlohmann::json> mi = this->_db->getDrvInfo(
    subtreeTypeToString( this->_subtree )
  , this->_system
  , p
  );
  if ( mi.has_value() )
    {
      return std::make_shared<CachedPackage>( mi.value() );
    }
  else
    {
      return nullptr;
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
