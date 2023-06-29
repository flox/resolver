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
  auto state = this->getDbState();
  nix::SQLiteStmt stmt;
  if ( this->_stability.has_value() )
    {
      stmt.create(
        state->db
      , "SELECT COUNT( subtree ) FROM DerivationInfos WHERE"
        "( subtree = ? ) AND ( system = ? ) AND ( path LIKE ? )"
      );
      auto query = stmt.use()( subtreeTypeToString( this->_subtree ) )
                             ( this->_system )
                             ( this->_stability.value() );
      if ( ! query.next() )
        {
          throw ResolverException(
            "DbPackageSet::size(): Failed to query table DerivationInfos."
          );
        }
      return query.getInt( 0 );
    }
  else
    {
      stmt.create( state->db
                 , "SELECT COUNT( subtree ) FROM DerivationInfos WHERE"
                   "( subtree = ? ) AND ( system = ? )"
                 );
      auto query = stmt.use()( subtreeTypeToString( this->_subtree ) )
                             ( this->_system );
      if ( ! query.next() )
        {
          throw ResolverException(
            "DbPackageSet::size(): Failed to query table DerivationInfos."
          );
        }
      return query.getInt( 0 );
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
