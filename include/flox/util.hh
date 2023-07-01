/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <cstdio>
#include <functional>
#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include <optional>
#include <vector>
#include <map>
#include <algorithm>


/* -------------------------------------------------------------------------- */

/* Required for `RawPackageSet' */
  template<>
struct std::hash<std::list<std::string_view>>
{
    std::size_t
  operator()( const std::list<std::string_view> & lst ) const noexcept
  {
    if ( lst.empty() ) { return 0; }
    auto it = lst.begin();
    std::size_t h1 = std::hash<std::string_view>{}( * it );
    for ( ; it != lst.cend(); ++it )
      {
        h1 = ( h1 >> 1 ) ^ ( std::hash<std::string_view>{}( *it ) << 1 );
      }
    return h1;
  }
};


/* -------------------------------------------------------------------------- */

  static bool
operator==( const std::list<std::string>      & lhs
          , const std::list<std::string_view> & rhs
          )
{
  return ( lhs.size() == rhs.size() ) && std::equal(
    lhs.cbegin(), lhs.cend(), rhs.cbegin()
  );
}

  static bool
operator==( const std::list<std::string_view> & lhs
          , const std::list<std::string>      & rhs
          )
{
  return ( lhs.size() == rhs.size() ) && std::equal(
    lhs.cbegin(), lhs.cend(), rhs.cbegin()
  );
}


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

static const std::list<std::string> defaultSystems = {
 "x86_64-linux", "aarch64-linux", "x86_64-darwin", "aarch64-darwin"
};

static const std::vector<std::string> defaultSubtrees = {
  "catalog", "packages", "legacyPackages"
};

static const std::vector<std::string> defaultCatalogStabilities = {
  "stable", "staging", "unstable"
};


/* -------------------------------------------------------------------------- */

  static inline bool
isPkgsSubtree( std::string_view attrName )
{
  return ( attrName == "legacyPackages" ) ||
         ( attrName == "packages" )       ||
         ( attrName == "catalog"  );
}


/* -------------------------------------------------------------------------- */

static nix::flake::LockFlags floxFlakeLockFlags = {
  .updateLockFile = false
, .writeLockFile  = false
, .applyNixConfig = false
};


/* -------------------------------------------------------------------------- */

template <typename T, template <typename, typename> class C>
  static inline bool
hasElement( const C<T, std::allocator<T>> & container, const T & e )
{
  return std::find( container.cbegin(), container.cend(), e ) !=
         container.cend();
}


/* -------------------------------------------------------------------------- */

  static inline bool
shouldSearchSystem( std::string_view system )
{
  return std::find( defaultSystems.cbegin(), defaultSystems.cend(), system ) !=
         defaultSystems.cend();
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
