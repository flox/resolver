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
#include "descriptor.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

std::optional<bool> isAbsAttrPathJSON( const nlohmann::json         & j );
std::optional<bool> isAbsAttrPath(     const std::vector<attr_part> & path );


/* -------------------------------------------------------------------------- */

bool isMatchingAttrPathPrefix( const std::vector<attr_part>      & prefix
                             , const std::vector<nix::SymbolStr> & path
                             );

bool isMatchingAttrPath( const std::vector<attr_part>      & prefix
                       , const std::vector<nix::SymbolStr> & path
                       );


/* -------------------------------------------------------------------------- */

struct PkgNameVersion {
  std::string                name;
  std::string                attrName;
  std::string                parsedName;
  std::string                parsedVersion;
  std::optional<std::string> pname;
  std::optional<std::string> version;
  // TODO
  // std::optional<std::string> semver;
  // bool isSemver() const { return this->semver.has_value(); }
};

/* AttrName, <drv>.pname, [parseDrvName( <drv>.name ).name] */
PkgNameVersion nameVersionAt( std::string_view              attrName
                            , nix::eval_cache::AttrCursor & pos
                            );


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

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
