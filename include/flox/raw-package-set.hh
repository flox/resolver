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

struct RawPackageSetIterator : public PackageSetIterator
{
  private:
    std::unordered_map<std::list<std::string_view>
                      , CachedPackage
                      >::iterator                  _end;
    std::unordered_map<std::list<std::string_view>
                      , CachedPackage
                      >::iterator                  _it;

  public:
    explicit RawPackageSetIterator(
      std::unordered_map<std::list<std::string_view>, CachedPackage> pkgs
    ) : _end( pkgs.end() ), _it( pkgs.begin() )
    {}

    explicit RawPackageSetIterator(
      std::unordered_map<std::list<std::string_view>, CachedPackage> pkgs
    , std::unordered_map<std::list<std::string_view>
                        , CachedPackage
                        >::iterator                                  it
    ) : _end( pkgs.end() ), _it( it )
    {}

    std::string_view getType() const override { return "raw"; }

      RawPackageSetIterator &
    operator++()
    {
      ++this->_it;
      return * this;
    }

      RawPackageSetIterator
    operator++( int )
    {
      RawPackageSetIterator tmp = * this;
      ++( * this );
      return tmp;
    }

      bool
    operator==( const RawPackageSetIterator & other ) const
    {
      return this->_it == other._it;
    }

      bool
    operator!=( const RawPackageSetIterator & other ) const
    {
      return this->_it != other._it;
    }

      reference
    operator*() const override
    {
      return this->_it->second;
    }

      pointer
    operator->() override
    {
      return & this->_it->second;
    }


};  /* End struct `RawPackageSetIterator' */



class RawPackageSet : public PackageSet<RawPackageSetIterator> {

  protected:
    std::unordered_map<std::list<std::string_view>, CachedPackage> _pkgs;

  private:
    subtree_type               _subtree;
    std::string                _system;
    std::optional<std::string> _stability;
    FloxFlakeRef               _ref;

  public:

    RawPackageSet(
      std::unordered_map<std::list<std::string_view>, CachedPackage> pkgs
    , subtree_type                                                   subtree
    , std::string_view                                               system
    , std::optional<std::string_view>                                stability
    , FloxFlakeRef                                                   ref
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
          return std::shared_ptr<Package>( (Package *) & search->second );
        }
    }

      nix::ref<Package>
    getRelPath( const std::list<std::string_view> & path ) override
    {
      return nix::ref<Package>( & this->_pkgs.at( path ) );
    }

      void
   addPackage( CachedPackage && p )
   {
     std::list<std::string_view> relPath;
     auto it = p._pathS.cbegin();
     it += ( p.getSubtreeType() == resolve::ST_CATALOG ) ? 3 : 2;
     for ( ; it != p._pathS.cend(); ++it ) { relPath.push_back( * it ); }
     this->_pkgs.emplace( std::move( relPath ), p );
   }

/* -------------------------------------------------------------------------- */

      RawPackageSetIterator
    begin() override
    {
      return RawPackageSetIterator( this->_pkgs );
    }


/* -------------------------------------------------------------------------- */

      RawPackageSetIterator
    end() override
    {
      return RawPackageSetIterator( this->_pkgs, this->_pkgs.end() );
    }



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
