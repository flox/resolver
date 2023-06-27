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
    virtual std::string_view getType()        const                         = 0;
    virtual std::string_view getSubtree()     const                         = 0;
    virtual std::string_view getSystem()      const                         = 0;
    virtual FloxFlakeRef     getRef()         const                         = 0;
    virtual FlakeRefWithPath getRefWithPath() const                         = 0;
    virtual std::size_t      size()                                         = 0;
    virtual bool             empty()                                        = 0;
    virtual bool             hasRelPath( std::list<std::string_view> path ) = 0;

    virtual std::optional<Package *> maybeGetRelPath(
      std::list<std::string_view> path
    ) = 0;

    struct PkgIter {
      using iterator_category = std::forward_iterator_tag;
      using value_type        = Package;
      using different_type    = std::ptrdiff_t;
      using pointer           = Package *;
      using reference         = Package &;

      explicit PkgIter() = default;
      explicit PkgIter( std::function<Package *()> next )
        : _next( std::move( next ) ), _ptr( nullptr )
      {
        this->_ptr = this->_next();
      }

        friend constexpr bool
      operator==( const PkgIter & lhs, const PkgIter & rhs ) noexcept
      {
        return lhs._ptr == rhs._ptr;
      }

        friend constexpr bool
      operator!=( const PkgIter & lhs, const PkgIter & rhs ) noexcept
      {
        return lhs._ptr != rhs._ptr;
      }

      PkgIter & operator++() { this->_ptr = this->_next(); return * this; }

        PkgIter
      operator++( int )
      {
        PkgIter tmp = * this;
        ++( * this );
        return tmp;
      }

            Package & operator*()        { return * this->_ptr; }
      const Package & operator*() const  { return * this->_ptr; }
            Package * operator->()       { return this->_ptr;   }

      protected:
        Package                    * _ptr  = nullptr;
        std::function<Package *()>   _next = [](){ return nullptr; };

    };

    virtual PkgIter begin()        = 0;
    virtual PkgIter end()          = 0;

    // TODO: your `_next' function would need to return `const Package *'
    //virtual PkgIter cbegin() const = 0;
    //virtual PkgIter cend()   const = 0;

};  /* End class `PackageSet' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
