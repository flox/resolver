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

struct PkgNameVersion {
  std::string                name;
  std::optional<std::string> parsedName;
  std::optional<std::string> parsedVersion;
  std::optional<std::string> pname;
  std::optional<std::string> version;
  // TODO
  // std::optional<std::string> semver;
  // bool isSemver() const { return this->semver.has_value(); }
  std::string getPname();
  std::string getVersion();
};

PkgNameVersion nameVersionAt( Cursor pos );


/* -------------------------------------------------------------------------- */

/* Return a ranked vector of satisfactory resolutions. */
std::vector<Resolved> resolve( const Inputs      & inputs
                             , const Preferences & preferences
                             , const Descriptor  & desc
                             );

/* Return the highest ranking resolution, or `std::nullopt'. */
std::optional<Resolved> resolveOne( const Inputs      & inputs
                                  , const Preferences & preferences
                                  , const Descriptor  & desc
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
