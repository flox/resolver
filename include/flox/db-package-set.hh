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
    ,       bool                                       trace     = false
    ) : DbPackageSet( flake
                    , std::make_shared<DrvDb>(
                        flake->getFingerprint()
                      , false
                      , false
                      , trace
                      )
                    , subtree
                    , system
                    , stability
                    )
    {}


/* -------------------------------------------------------------------------- */

      nix::flake::Fingerprint
    getFingerprint() const
    {
      return this->_flake->getFingerprint();
    }


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

    bool        hasRelPath( const std::list<std::string_view> & path ) override;
    std::size_t size() override;


/* -------------------------------------------------------------------------- */

    struct iterator
    {
      private:
        nix::SQLiteStmt::Use _query;
        nlohmann::json       _val;
        bool                 _hasNext = true;

      public:
        iterator()
          : _query( nix::SQLiteStmt().use() ), _val(), _hasNext( false )
        {}

        explicit iterator( nix::SQLiteStmt::Use query )
          : _query( query )
        {
          ++( * this );
        }

        std::string_view getType() const { return "db"; }

          iterator &
        operator++()
        {
          if ( this->_hasNext )
            {
              this->_hasNext = this->_query.next();
              this->_val = infoFromQuery( this->_query );
            }
          else
            {
              this->_val = nlohmann::json();  /* set `null' */
            }
          return * this;
        }

          iterator
        operator++( int )
        {
          iterator tmp = * this;
          ++( * this );
          return tmp;
        }

          bool
        operator==( const iterator & other ) const
        {
          return this->_val == other._val;
        }

          bool
        operator!=( const iterator & other ) const
        {
          return ! ( ( * this ) == other );
        }

    };  /* End struct `DbPackageSet::iterator' */


/* -------------------------------------------------------------------------- */

      iterator
    begin()
    {
      return iterator(
        this->_db->useDrvInfos( subtreeTypeToString( this->_subtree )
                              , this->_system
                              )
      );
    }

      iterator
    end()
    {
      return iterator();
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
