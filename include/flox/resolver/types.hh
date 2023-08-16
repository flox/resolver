/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <iterator>
#include <cstddef>
#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <nix/eval-cache.hh>
#include <unordered_map>
#include <unordered_set>
#include "flox/resolver/exceptions.hh"
#include "flox/resolver/util.hh"
#include <queue>
#include <any>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

using FloxFlakeRef = nix::FlakeRef;

using Cursor      = nix::ref<nix::eval_cache::AttrCursor>;
using CursorPos   = std::pair<Cursor, std::vector<nix::Symbol>>;
using MaybeCursor = std::shared_ptr<nix::eval_cache::AttrCursor>;


/* -------------------------------------------------------------------------- */

/**
 * A queue of cursors used to stash sub-attrsets that need to be searched
 * recursively in various iterators.
 */
using todo_queue = std::queue<Cursor, std::list<Cursor>>;


/* -------------------------------------------------------------------------- */

class Descriptor;
class Package;
class DrvDb;


/* -------------------------------------------------------------------------- */

typedef enum {
  ST_NONE     = 0
, ST_PACKAGES = 1
, ST_LEGACY   = 2
, ST_CATALOG  = 3
} subtree_type;

NLOHMANN_JSON_SERIALIZE_ENUM( subtree_type, {
  { ST_NONE,     nullptr          }
, { ST_PACKAGES, "packages"       }
, { ST_LEGACY,   "legacyPackages" }
, { ST_CATALOG,  "catalog"        }
} )

subtree_type     parseSubtreeType( std::string_view subtree );
std::string_view subtreeTypeToString( const subtree_type & st );


/* -------------------------------------------------------------------------- */

typedef enum {
  SY_NONE     = 0
, SY_STABLE   = 1
, SY_STAGING  = 2
, SY_UNSTABLE = 3
} stability_type;

NLOHMANN_JSON_SERIALIZE_ENUM( stability_type, {
  { SY_NONE,     nullptr    }
, { SY_STABLE,   "stable"   }
, { SY_STAGING,  "staging"  }
, { SY_UNSTABLE, "unstable" }
} )


/* -------------------------------------------------------------------------- */

using attr_part  = std::variant<std::nullptr_t, std::string>;
using attr_parts = std::vector<attr_part>;

struct AttrPathGlob {

  attr_parts path = {};

  static AttrPathGlob fromStrings( const std::vector<std::string>      & pp );
  static AttrPathGlob fromStrings( const std::vector<std::string_view> & pp );
  static AttrPathGlob fromJSON(    const nlohmann::json                & pp );

  AttrPathGlob()                        = default;
  AttrPathGlob( const AttrPathGlob &  ) = default;
  AttrPathGlob(       AttrPathGlob && ) = default;
  AttrPathGlob( const attr_parts &  pp );
  AttrPathGlob(       attr_parts && pp );

  AttrPathGlob & operator=( const AttrPathGlob & ) = default;

  std::string    toString() const;
  nlohmann::json toJSON()   const;

  bool isAbsolute() const;
  bool hasGlob()    const;
  /* Replace second element ( if present ) with `nullptr' glob. */
  void coerceRelative();
  /* Replace second element ( if present ) with `nullptr' glob. */
  void coerceGlob();

  bool globEq(     const AttrPathGlob & other ) const;
  bool operator==( const AttrPathGlob & other ) const;

  size_t size() const { return this->path.size(); }

};

void from_json( const nlohmann::json & j,       AttrPathGlob & path );
void to_json(         nlohmann::json & j, const AttrPathGlob & path );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- */

  template<>
struct std::hash<flox::resolve::AttrPathGlob>
{
    std::size_t
  operator()( const flox::resolve::AttrPathGlob & k ) const noexcept
  {
    if ( k.path.size() < 1 ) { return 0; }
    std::size_t h1 = std::hash<std::string>{}(
      std::get<std::string>( k.path[0] )
    );
    for ( size_t i = 1; i < k.path.size(); ++i )
      {
        if ( std::holds_alternative<std::string>( k.path[1] ) )
          {
            std::string p = std::get<std::string>( k.path[i] );
            if ( p != "{{system}}" )
              {
                std::size_t h2 = std::hash<std::string>{}( p );
                h1 = ( h1 >> 1 ) ^ ( h2 << 1 );
              }
          }
      }
    return h1;
  }
};


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
