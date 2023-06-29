/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <optional>
#include "flox/package.hh"

/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

struct FlakeRefWithPath {
  FloxFlakeRef           ref;
  std::list<std::string> path;
  std::string            toString() const;
};

std::string to_string( const FlakeRefWithPath & rp );


/* -------------------------------------------------------------------------- */

class PackageSet {

  public:
    virtual std::string_view getType()    const = 0;
    virtual subtree_type     getSubtree() const = 0;
    virtual std::string_view getSystem()  const = 0;
    virtual FloxFlakeRef     getRef()     const = 0;
    virtual std::size_t      size()             = 0;

    virtual bool empty() { return this->size() <= 0; }

    virtual bool hasRelPath( const std::list<std::string_view> & path ) = 0;

    virtual std::optional<std::string_view> getStability() const = 0;

      virtual std::list<std::string_view>
    getPrefix() const
    {
      auto s = this->getStability();
      if ( s.has_value() )
        {
          return {
            subtreeTypeToString( this->getSubtree() )
          , this->getSystem()
          , s.value()
          };
        }
      else
        {
          return {
            subtreeTypeToString( this->getSubtree() )
          , this->getSystem()
          };
        }
    }

      virtual FlakeRefWithPath
    getRefWithPath() const
    {
      FlakeRefWithPath rp = { this->getRef(), {} };
      for ( auto & p : this->getPrefix() ) { rp.path.emplace_back( p ); }
      return rp;
    }

    virtual std::shared_ptr<Package> maybeGetRelPath(
      const std::list<std::string_view> & path
    ) = 0;

      virtual nix::ref<Package>
    getRelPath( const std::list<std::string_view> & path )
    {
      std::shared_ptr<Package> p = this->maybeGetRelPath( path );
      if ( p == nullptr )
        {
          std::string msg( "PackageSet::getRelPath(): No such path '" );
          msg += this->getRefWithPath().toString();
          msg += "'.";
          throw ResolverException( msg.c_str() );
        }
      return (nix::ref<Package>) p;
    }

    template<bool IS_CONST> struct iterator_impl;
    using iterator       = iterator_impl<false>;
    using const_iterator = iterator_impl<true>;

      template<bool IS_CONST>
    struct iterator_impl
    {
      using iterator_category = std::forward_iterator_tag;
      using value_type        =
        typename std::conditional<IS_CONST, const Package, Package>::type;
      using pointer   = std::shared_ptr<value_type>;
      using reference = nix::ref<value_type>;
      public:
        std::string_view getType() const;
    };  /* End struct `PackageSet::iterator_impl' */

    virtual iterator       begin()        = 0;
    virtual iterator       end()          = 0;
    virtual const_iterator begin()  const = 0;
    virtual const_iterator end()    const = 0;
    virtual const_iterator cbegin() const { return this->begin(); }
    virtual const_iterator cend()   const { return this->end();   }

};  /* End class `PackageSet' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
