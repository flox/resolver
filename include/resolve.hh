/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <unordered_map>
#include <unordered_set>
#include "flox/exceptions.hh"
#include "flox/types.hh"
#include "flox/predicates.hh"
#include "flox/resolver-state.hh"
#include "descriptor.hh"


/* -------------------------------------------------------------------------- */

/* This is passed in by `make' and is set by `<resolver>/version' */
#ifndef FLOX_RESOLVER_VERSION
#  define FLOX_RESOLVER_VERSION  "NO.VERSION"
#endif


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

bool isMatchingAttrPathPrefix( const AttrPathGlob                & prefix
                             , const std::vector<nix::SymbolStr> & path
                             );

bool isMatchingAttrPath( const AttrPathGlob                & prefix
                       , const std::vector<nix::SymbolStr> & path
                       );


/* -------------------------------------------------------------------------- */

std::list<Resolved> resolve_V2(       ResolverState & rs
                              , const Descriptor    & desc
                              ,       bool            one  = false
                              );

std::optional<Resolved> resolveOne_V2(       ResolverState & rs
                                     , const Descriptor    & desc
                                     );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
