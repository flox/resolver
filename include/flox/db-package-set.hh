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
                      , false                   /* create */
                      , false                   /* write  */
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

    std::shared_ptr<Package> maybeGetRelPath(
      const std::list<std::string_view> & path
    ) override;


/* -------------------------------------------------------------------------- */

    struct const_iterator
    {
      using value_type = const CachedPackage;
      using reference  = value_type &;
      using pointer    = nix::ref<value_type>;

      private:
        std::optional<nix::SQLiteStmt::Use> _query;
        std::shared_ptr<CachedPackage>      _ptr;

      public:
        const_iterator()
          : _ptr( nullptr ), _query( std::nullopt )
        {}

        explicit const_iterator( nix::SQLiteStmt::Use query )
          : _query( query ), _ptr( nullptr )
        {
          ++( * this );
        }

        std::string_view getType() const { return "db"; }

          const_iterator &
        operator++()
        {
          if ( this->_query.has_value() && this->_query.value().next() )
            {
              this->_ptr = std::make_shared<CachedPackage>(
                infoFromQuery( this->_query.value() )
              );
            }
          else
            {
              this->_query = std::nullopt;
              this->_ptr   = nullptr;
            }
          return * this;
        }

          const_iterator
        operator++( int )
        {
          const_iterator tmp = * this;
          ++( * this );
          return tmp;
        }

          bool
        operator==( const const_iterator & other ) const
        {
          return this->_ptr == other._ptr;
        }

          bool
        operator!=( const const_iterator & other ) const
        {
          return  this->_ptr != other._ptr;
        }

        reference operator*()  const { return * this->_ptr; }

          pointer
        operator->()
        {
          return (nix::ref<const CachedPackage>) this->_ptr;
        }

    };  /* End struct `DbPackageSet::iterator' */


/* -------------------------------------------------------------------------- */

      const_iterator
    begin() const
    {
      if ( this->_subtree == ST_CATALOG )
        {
          return const_iterator(
            this->_db->useDrvInfosStability( this->_system
                                           , this->_stability.value()
                                           )
          );
        }
      else
        {
          return const_iterator(
            this->_db->useDrvInfos( subtreeTypeToString( this->_subtree )
                                  , this->_system
                                  )
          );
        }
    }

      const_iterator
    end() const
    {
      return const_iterator();
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
