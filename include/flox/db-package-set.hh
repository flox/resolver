/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include "flox/package-set.hh"
#include "flox/drv-cache.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class DbPackageSet : public PackageSet {

  private:

    subtree_type                             _subtree;
    std::string                              _system;
    std::optional<std::string>               _stability;
    std::shared_ptr<nix::flake::LockedFlake> _flake;
    std::shared_ptr<DrvDb>                   _db;

      nix::Sync<DrvDb::State>::Lock
    getDbState()
    {
      return this->_db->getDbState();
    }


/* -------------------------------------------------------------------------- */

  public:

    // TODO: `readonly' flag for `db'

    DbPackageSet(
            std::shared_ptr<nix::flake::LockedFlake>   flake
    ,       std::shared_ptr<DrvDb>                     db
    , const subtree_type                             & subtree
    ,       std::string_view                           system
    , const std::optional<std::string_view>          & stability = std::nullopt
    ) : _subtree( subtree )
      , _system( system )
      , _stability( stability )
      , _flake( flake )
      , _db( db )
    {}

    DbPackageSet(
            std::shared_ptr<nix::flake::LockedFlake>   flake
    , const subtree_type                             & subtree
    ,       std::string_view                           system
    , const std::optional<std::string_view>          & stability = std::nullopt
    ) : DbPackageSet( flake
                    , std::make_shared<DrvDb>( flake->getFingerprint() )
                    , subtree
                    , system
                    , stability
                    )
    {}


/* -------------------------------------------------------------------------- */

    std::string_view getType()    const override { return "db";           }
    subtree_type     getSubtree() const override { return this->_subtree; }
    std::string_view getSystem()  const override { return this->_system;  }

      std::optional<std::string_view>
    getStability() const override
    {
      if ( this->_stability.has_value() ) { return this->_stability; }
      else                                { return std::nullopt;     }
    }

      FloxFlakeRef
    getRef() const override
    {
      return this->_flake->flake.lockedRef;
    }


/* -------------------------------------------------------------------------- */

    // TODO: change `DrvDb' to accept `std::list<std::string_view'
      bool
    hasRelPath( const std::list<std::string_view> & path ) override
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
    size() override
    {
      return this->_db->doSQLite( [&]() {
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
      } );
    }


/* -------------------------------------------------------------------------- */

};  /* End class `DbPackageSet' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
