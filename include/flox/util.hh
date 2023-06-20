/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <cstdio>
#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include "resolve.hh"
#include <optional>
#include <vector>
#include <map>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

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

std::list<Resolved> & mergeResolvedByAttrPathGlob( std::list<Resolved> & lst );


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
