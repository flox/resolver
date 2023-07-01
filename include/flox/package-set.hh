/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <concepts>
#include <span>
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

  template<typename T, typename U>
concept same_as_unqual =
    std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

  template<typename From, typename To>
concept convertible_to_unqual =
  std::convertible_to<std::remove_cvref_t<From>, std::remove_cvref_t<To>>;

  template<class Derived, class Base>
concept derived_from_unqual =
  std::derived_from<std::remove_cvref_t<Derived>, Base>;


/* -------------------------------------------------------------------------- */

  template <class T>
constexpr bool is_unqual_package() noexcept { return false; }
  template <derived_from_unqual<Package> T>
constexpr bool is_unqual_package() noexcept { return true; }


/* -------------------------------------------------------------------------- */

template <class T> concept PackageLike =
    derived_from_unqual<T, Package>                    ||
    convertible_to_unqual<T, std::shared_ptr<Package>> ||
    convertible_to_unqual<T, nix::ref<Package>>;


  template <class Iter>
concept PackageSetIterator =
  std::input_iterator<Iter> && PackageLike<std::iter_value_t<Iter>>;


/* -------------------------------------------------------------------------- */

  template<class T>
concept PackageSetLike =
  requires( const T p )
  {
    { p.getType()        } -> std::convertible_to<std::string_view>;
    { p.getSubtree()     } -> std::convertible_to<subtree_type>;
    { p.getSystem()      } -> std::convertible_to<std::string_view>;
    { p.getRef()         } -> std::convertible_to<FloxFlakeRef>;
    { p.size()           } -> std::convertible_to<FloxFlakeRef>;
    { p.empty()          } -> std::same_as<bool>;
    { p.getStability()   } -> std::same_as<std::optional<std::string_view>>;
    { p.getPrefix()      } -> std::convertible_to<std::span<std::string_view>>;
    { p.getRefWithPath() } -> std::convertible_to<FlakeRefWithPath>;
  }
  &&
  requires( T p )
  {
    { p.hasRelPath( std::list<std::string_view>() ) } -> std::same_as<bool>;
    {
      p.getRelPath( std::span<std::string_view>() )
    } -> derived_from_unqual<Package>;
  };


/* -------------------------------------------------------------------------- */

  template<PackageSetIterator Iter>
class PackageSet {

  public:
    virtual std::string_view getType()    const = 0;
    virtual subtree_type     getSubtree() const = 0;
    virtual std::string_view getSystem()  const = 0;
    virtual FloxFlakeRef     getRef()     const = 0;
    virtual std::size_t      size()             = 0;

    virtual bool empty() { return this->size() <= 0; }

    virtual bool hasRelPath( const std::span<std::string_view> & path ) = 0;

    virtual std::optional<std::string_view> getStability() const = 0;

      virtual std::span<std::string_view>
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
    getRelPath( const std::span<std::string_view> & path )
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

    virtual Iter begin() = 0;
    virtual Iter end()   = 0;

};  /* End class `PackageSet' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
