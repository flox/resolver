/* ========================================================================== *
 *
 * @file flox/raw-package-set.hh
 *
 * @brief Declares a package set comprised of metadata stored "in memory".
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <optional>
#include "flox/util.hh"
#include "flox/package-set.hh"
#include "flox/raw-package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

/** maps attribute-paths to packages */
using RawPackageMap =
  std::unordered_map<std::list<std::string_view>, nix::ref<RawPackage>>;


/* -------------------------------------------------------------------------- */

/**
 * A package set comprised of metadata stored "in memory".
 * This is the simplest implementation of @a PackageSet and is primarily used
 * for testing.
 */
class RawPackageSet : public PackageSet {

  protected:
    RawPackageMap _pkgs;  /**< attr-path -> package data */

  private:
    subtree_type               _subtree;
    std::string                _system;
    std::optional<std::string> _stability;
    FloxFlakeRef               _ref;  /** indicates package set's "source" */

  public:

    /**
     * Constructs an package set associated a flake and attr-path prefix.
     * @param pkgs attr-path -> package data.
     * @param subtree flake output "subtree" the package set comes from.
     * @param system architecture/platform the package set comes from.
     * @param stability `flox` _stability_ category the package set comes from.
     *                  ( optional: `catalog` @a subtree only )
     */
    RawPackageSet(
      RawPackageMap                   pkgs
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

    /**
     * PackageSet "type" represented as a simple string.
     * This is used for error messages and working with abstract @a PackageSet
     * references in generic utility functions.
     */
    std::string_view getType() const override { return "raw"; }

    /** @return the flake output "subtree" associated with the package set. */
    subtree_type getSubtree() const override { return this->_subtree; }

    /** @return the architecture/platform associated with the package set. */
    std::string_view getSystem() const override { return this->_system; }

    /**
     * @return the flake reference assocaited with the package set indicating
     *         its source.
     */
    FloxFlakeRef getRef() const override { return this->_ref; }

    /**
     * @return For package sets in a `catalog` @a subtree, returns the
     *         associated `flox` _stability_ associated with the package set.
     *         For non-catalog package sets, returns @a std::nullopt.
     */
      std::optional<std::string_view>
    getStability() const override
    {
      if ( this->_stability.has_value() ) { return this->_stability; }
      else                                { return std::nullopt;     }
    }

    /** @return the number of packages in the package set. */
    std::size_t size() override { return this->_pkgs.size(); }
    /** @return the number of packages in the package set. */
    std::size_t size() const { return this->_pkgs.size(); }
    /** @return true iff the package set has no packages. */
    bool empty() override { return this->_pkgs.empty(); }
    /** @return true iff the package set has no packages. */
    bool empty() const { return this->_pkgs.empty(); }

    /**
     * Predicate which checks to see if the package set has a package at the
     * relative path @a path.
     * @param path a relative attribute path ( with no subtree, system,
     *             or stability components ) to search for.
     * @return true iff the package set has a package at @a path.
     */
      bool
    hasRelPath( const std::list<std::string_view> & path ) override
    {
      return this->_pkgs.find( path ) != this->_pkgs.cend();
    }

    /**
     * Attempts to get package metadata associated with the relative path
     * @a path if it exists.
     * @param path a relative attribute path ( with no subtree, system,
     *             or stability components ) to search for.
     * @return nullptr if the package set does not contain a package at @a path,
     *         otherwise a pointer to the requested package metadata.
     */
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

    /**
     * Gets package metadata associated with the relative path @a path.
     * Throws an error if the package set is missing the requested metadata.
     * @param path a relative attribute path ( with no subtree, system,
     *             or stability components ) to search for.
     * @return a non-null pointer to the requested package metadata.
     */
      nix::ref<Package>
    getRelPath( const std::list<std::string_view> & path ) override
    {
      return this->_pkgs.at( path );
    }

    /**
     * Adds package metadata to the package set.
     * @a p is assumed to have an attribute path which is consistent
     * with @a this package set.
     * @param p package metadata to be added.
     */
      void
   addPackage( RawPackage && p )
   {
     std::list<std::string_view> relPath;
     auto it = p._pathS.cbegin();
     it += ( p.getSubtreeType() == resolve::ST_CATALOG ) ? 3 : 2;
     for ( ; it != p._pathS.cend(); ++it ) { relPath.push_back( * it ); }
     this->_pkgs.emplace(
       std::move( relPath )
     , nix::make_ref<RawPackage>( p )
     );
   }

/* -------------------------------------------------------------------------- */

    template<bool IS_CONST> struct iterator_impl;
    using iterator       = iterator_impl<false>;
    using const_iterator = iterator_impl<true>;

    /** Iterators used to visit members of a @a RawPackageSet. */
      template<bool IS_CONST>
    struct iterator_impl
    {
      using value_type =
        std::conditional<IS_CONST, const RawPackage, RawPackage>::type;
      using reference  = value_type &;
      using pointer    = nix::ref<value_type>;

      using container_type =
        std::conditional<IS_CONST, const RawPackageMap
                                 , RawPackageMap
                        >::type;

      using wrapped_iter_type =
        std::conditional<IS_CONST, RawPackageMap::const_iterator
                                 , RawPackageMap::iterator
                        >::type;

      private:
        container_type              * _pkgs;
        wrapped_iter_type             _end;
        wrapped_iter_type             _it;
        std::shared_ptr<RawPackage>   _ptr;

      public:

        iterator_impl( container_type * pkgs )
          : _end( pkgs->end() )
          , _it( pkgs->begin() )
          , _pkgs( pkgs )
          , _ptr( nullptr )
        {
          if ( this->_it != this->_end ) { this->_ptr = this->_it->second; }
        }

        iterator_impl( container_type * pkgs, wrapped_iter_type it )
          : _end( pkgs->end() ), _it( it ), _pkgs( pkgs ), _ptr( nullptr )
        {
          if ( this->_it != this->_end ) { this->_ptr = this->_it->second; }
        }

        std::string_view getType() const { return "raw"; }

        /**
         * Increment the iterator one position forward.
         * @return the incremented iterator.
         */
          iterator_impl &
        operator++()
        {
          if ( this->_it == this->_end ) { return * this; }
          ++this->_it;
          if ( this->_it == this->_end ) { this->_ptr = nullptr; }
          else                           { this->_ptr = this->_it->second; }
          return * this;
        }

        /**
         * Increment the iterator one position forward.
         * @return the orginal iterator.
         */
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

        /** @return a reference to the @a RawPackage at the current position. */
        reference operator*()  const { return * this->_ptr; }
        /** @return a pointer to the @a RawPackage at the current position. */
        pointer   operator->()       { return this->_ptr; }

        friend iterator;
        friend const_iterator;

    };  /* End struct `RawPackageSet::iterator_impl' */


/* -------------------------------------------------------------------------- */

    /** @return read/write iterator at the beginning of the container. */
    iterator begin() { return iterator( & this->_pkgs ); }
    /** @return sentinel value used to represent the "end" of the container. */
    iterator end() { return iterator( & this->_pkgs, this->_pkgs.end() ); }

    /** @return read-only iterator at the beginning of the container. */
    const_iterator begin() const { return const_iterator( & this->_pkgs ); }

    /** @return sentinel value used to represent the "end" of the container. */
      const_iterator
    end() const
    {
      return const_iterator( & this->_pkgs, this->_pkgs.cend() );
    }

    /** @return read-only iterator at the beginning of the container. */
    const_iterator cbegin() const { return this->begin(); }
    /** @return sentinel value used to represent the "end" of the container. */
    const_iterator cend()   const { return this->end();   }


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
