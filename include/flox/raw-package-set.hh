/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <optional>
#include "flox/util.hh"
#include "flox/package-set.hh"
#include "flox/drv-cache.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

using CachedPackageMap =
  std::unordered_map<std::list<std::string_view>, nix::ref<CachedPackage>>;


/* -------------------------------------------------------------------------- */

class RawPackageSet : public PackageSet {

  protected:
    CachedPackageMap _pkgs;

  private:
    subtree_type               _subtree;
    std::string                _system;
    std::optional<std::string> _stability;
    FloxFlakeRef               _ref;

  public:

    RawPackageSet(
      CachedPackageMap                pkgs
    , subtree_type                    subtree
    , std::string_view                system
    , std::optional<std::string_view> stability
    , FloxFlakeRef                    ref
    ) : _pkgs( pkgs )
      , _subtree( subtree )
      , _system( system )
      , _stability( stability.has_value()
                      ? std::make_optional( std::string( stability.value() ) )
                      : std::nullopt
                  )
      , _ref( ref )
    {}

    std::string_view getType()    const override { return "raw";          }
    subtree_type     getSubtree() const override { return this->_subtree; }
    std::string_view getSystem()  const override { return this->_system;  }
    FloxFlakeRef     getRef()     const override { return this->_ref;     }

      std::optional<std::string_view>
    getStability() const override
    {
      if ( this->_stability.has_value() ) { return this->_stability; }
      else                                { return std::nullopt;     }
    }

    std::size_t size()  override { return this->_pkgs.size();  }
    std::size_t size()  const    { return this->_pkgs.size();  }
    bool        empty() override { return this->_pkgs.empty(); }
    bool        empty() const    { return this->_pkgs.empty(); }

      bool
    hasRelPath( const std::list<std::string_view> & path ) override
    {
      return this->_pkgs.find( path ) != this->_pkgs.cend();
    }

      std::shared_ptr<Package>
    maybeGetRelPath( const std::list<std::string_view> & path ) override
    {
      auto search = this->_pkgs.find( path );
      if ( search == this->_pkgs.cend() )
        {
          return nullptr;
        }
      else
        {
          return search->second.get_ptr();
        }
    }

      nix::ref<Package>
    getRelPath( const std::list<std::string_view> & path ) override
    {
      return this->_pkgs.at( path );
    }

      void
   addPackage( CachedPackage && p )
   {
     std::list<std::string_view> relPath;
     auto it = p._pathS.cbegin();
     it += ( p.getSubtreeType() == resolve::ST_CATALOG ) ? 3 : 2;
     for ( ; it != p._pathS.cend(); ++it ) { relPath.push_back( * it ); }
     this->_pkgs.emplace(
       std::move( relPath )
     , nix::make_ref<CachedPackage>( p )
     );
   }

/* -------------------------------------------------------------------------- */

    template<bool IS_CONST> struct iterator_impl;
    using iterator       = iterator_impl<false>;
    using const_iterator = iterator_impl<true>;

      template<bool IS_CONST>
    struct iterator_impl
    {
      using value_type =
        std::conditional<IS_CONST, const CachedPackage, CachedPackage>;
      using reference  = value_type &;
      using pointer    = nix::ref<value_type>;

      using container_type =
        std::conditional<IS_CONST, const CachedPackageMap, CachedPackageMap>;

      using wrapped_iter_type =
        std::conditional<IS_CONST, CachedPackageMap::const_iterator
                                 , CachedPackageMap::iterator
                        >;

      private:
        container_type                   * _pkgs;
        CachedPackageMap::const_iterator   _end;
        wrapped_iter_type                  _it;
        nix::ref<CachedPackage>            _ptr;

      public:

        iterator_impl( container_type * pkgs )
          : _end( pkgs->cend() )
          , _it( pkgs->begin() )
          , _pkgs( pkgs )
          , _ptr( nullptr )
        {
          this->_ptr = this->_it->second;
        }

        iterator_impl( container_type * pkgs, wrapped_iter_type it )
          : _end( pkgs->cend() ), _it( it ), _pkgs( pkgs ), _ptr( it->second )
        {}

        std::string_view getType() const { return "raw"; }

          iterator_impl &
        operator++()
        {
          ++this->_it;
          this->_ptr = this->_it->second;
          return * this;
        }

          iterator_impl
        operator++( int )
        {
          iterator_impl tmp = * this;
          ++( * this );
          return tmp;
        }

          bool
        operator==( const iterator & other ) const
        {
          return this->_ptr == other._ptr;
        }
          bool
        operator!=( const iterator & other ) const
        {
          return this->_ptr != other._ptr;
        }
          bool
        operator==( const const_iterator & other ) const
        {
          return this->_ptr == other._ptr;
        }
          bool
        operator!=( const const_iterator & other ) const
        {
          return this->_ptr != other._ptr;
        }

        reference operator*() const { return * this->_ptr; }
        pointer   operator->()      { return this->_ptr; }

        friend iterator;
        friend const_iterator;

    };  /* End struct `RawPackageSet::iterator_impl' */


/* -------------------------------------------------------------------------- */

      iterator
    begin()
    {
      return iterator( (CachedPackageMap *) & this->_pkgs );
    }

    iterator end()   { return iterator( & this->_pkgs, this->_pkgs.end() ); }

    const_iterator begin() const { return const_iterator( & this->_pkgs ); }

      const_iterator
    end() const
    {
      return const_iterator( & this->_pkgs, this->_pkgs.cend() );
    }

    const_iterator cbegin() const { this->begin(); }
    const_iterator cend()   const { this->end();   }


/* -------------------------------------------------------------------------- */

};  /* End class `RawPackageSet' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
