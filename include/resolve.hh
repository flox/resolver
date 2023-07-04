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
#include "descriptor.hh"


/* -------------------------------------------------------------------------- */

#define LIBFLOX_RESOLVE_VERSION  "0.2.0"


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
