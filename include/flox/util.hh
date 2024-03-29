/* ========================================================================== *
 *
 * @file flox/util.hh
 *
 * @brief Miscellaneous helper functions.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <cstdio>
#include <functional>
#include <optional>
#include <vector>
#include <map>
#include <algorithm>
#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>


/* -------------------------------------------------------------------------- */

  template<>
struct std::hash<std::list<std::string_view>>
{
  /**
   * Generate a unique hash for a list of strings.
   * @see flox::resolve::RawPackageMap
   * @param lst a list of strings.
   * @return a unique hash based on the contents of `lst` members.
   */
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

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunused-function"
#endif  /* ifdef __clang__ */

#ifdef __GNUG__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-function"
#endif  /* ifdef __GNUG__ */

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


#ifdef __clang__
#  pragma clang diagnostic pop
#endif  /* ifdef __clang__ */

#ifdef __GNUG__
#  pragma GCC diagnostic pop
#endif  /* ifdef __GNUG__ */


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

/** Systems to resolve/search in. */
static const std::list<std::string> defaultSystems = {
 "x86_64-linux", "aarch64-linux", "x86_64-darwin", "aarch64-darwin"
};

/** `flake' subtrees to resolve/search in. */
static const std::vector<std::string> defaultSubtrees = {
  "catalog", "packages", "legacyPackages"
};

/** Catalog stabilities to resolve/search in. */
static const std::vector<std::string> defaultCatalogStabilities = {
  "stable", "staging", "unstable"
};


/* -------------------------------------------------------------------------- */

/**
 * Predicate which checks to see if a string is a `flake' "subtree" name.
 * @return true iff `attrName` is one of "legacyPackages", "packages",
 *         or "catalog".
 */
  static inline bool
isPkgsSubtree( std::string_view attrName )
{
  return ( attrName == "legacyPackages" ) ||
         ( attrName == "packages" )       ||
         ( attrName == "catalog"  );
}


/* -------------------------------------------------------------------------- */

/**
 * Default flags used when locking flakes.
 * - Disable `updateLockFile` and read existing lockfiles directly.
 * - Disable `writeLockFile` to avoid writing generated lockfiles to the
 *   filesystem; this will only occur if there is no existing lockfile.
 */
static const nix::flake::LockFlags floxFlakeLockFlags = {
  .recreateLockFile      = false         /* default */
, .updateLockFile        = false
, .writeLockFile         = false
, .useRegistries         = std::nullopt  /* default */
, .applyNixConfig        = false         /* default */
, .allowUnlocked         = true          /* default */
, .commitLockFile        = false         /* default */
, .referenceLockFilePath = std::nullopt  /* default */
, .outputLockFilePath    = std::nullopt  /* default */
, .inputOverrides        = {}            /* default */
, .inputUpdates          = {}            /* default */
};


/* -------------------------------------------------------------------------- */

/**
 * Predicate which indicates whether a `storePath' is "substitutable".
 * @param storePath an absolute path in the `/nix/store'.
 *        This should be an `outPath' and NOT a `drvPath' in most cases.
 * @return true iff `storePath' is cached in a remote `nix' store and can be
 *              copied without being "rebuilt" from scratch.
 */
bool isSubstitutable( std::string_view storePath );


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
