/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

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
#include "flox/exceptions.hh"

/* In `nix' 2.14.0 they moved some class declarations around, so this
 * selects the correct header so that we can refer to `InstallableFlake'. */
#ifdef HAVE_INSTALLABLE_FLAKE
#  include <nix/installable-flake.hh>
#else
#  include <nix/installables.hh>
#endif


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

static const std::list<std::string> defaultSystems = {
 "x86_64-linux", "aarch64-linux", "x86_64-darwin", "aarch64-darwin"
};


/* -------------------------------------------------------------------------- */

using FloxFlakeRef = nix::FlakeRef;
using input_pair   =
  std::pair<std::string, std::shared_ptr<nix::flake::LockedFlake>>;

using attr_part  = std::variant<std::nullptr_t, std::string>;
using attr_parts = std::vector<attr_part>;

using Cursor      = nix::ref<nix::eval_cache::AttrCursor>;
using CursorPos   = std::pair<Cursor, std::vector<nix::Symbol>>;
using MaybeCursor = std::shared_ptr<nix::eval_cache::AttrCursor>;


/* -------------------------------------------------------------------------- */

typedef enum { ST_PACKAGES, ST_LEGACY, ST_CATALOG } subtree_type;


/* -------------------------------------------------------------------------- */

struct AttrPathGlob {

  attr_parts path;

  static AttrPathGlob fromStrings( const std::vector<std::string>      & path );
  static AttrPathGlob fromStrings( const std::vector<std::string_view> & path );
  static AttrPathGlob fromJSON(    const nlohmann::json                & path );

  AttrPathGlob() = default;
  AttrPathGlob( const attr_parts & path );

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

};

void from_json( const nlohmann::json & j,       AttrPathGlob & path );
void to_json(         nlohmann::json & j, const AttrPathGlob & path );


/* -------------------------------------------------------------------------- */

class Inputs {
  private:
    void init( const nlohmann::json & j );

  public:
    std::unordered_map<std::string, FloxFlakeRef> inputs;

    Inputs() = default;
    Inputs( const nlohmann::json & j ) { this->init( j ); }

    bool           has( std::string_view id ) const;
    FloxFlakeRef   get( std::string_view id ) const;
    nlohmann::json toJSON()                   const;
};


void from_json( const nlohmann::json & j,       Inputs & i );
void to_json(         nlohmann::json & j, const Inputs & i );


/* -------------------------------------------------------------------------- */

using PkgPredicate = std::function<bool(
                             Cursor
                     , const std::vector<nix::Symbol> &
                     )>;

  static bool
defaultPkgPredicate(       Cursor                     pos
                   , const std::vector<nix::Symbol> & path
                   )
{
  return true;
}


/* -------------------------------------------------------------------------- */

static const std::vector<std::string> defaultCatalogStabilities = {
  "stable", "staging", "unstable"
};

static const std::vector<std::string> defaultAttrPathPrefixes = {
  "catalog", "packages", "legacyPackages"
};


struct Preferences {
  std::vector<std::string> inputs;

  std::unordered_map<std::string, std::vector<std::string>> stabilities;
  std::unordered_map<std::string, std::vector<std::string>> prefixes;

  bool semverPreferPreReleases = false;
  bool allowUnfree             = true;
  bool allowBroken             = false;

  std::optional<std::unordered_set<std::string>> allowedLicenses;

  Preferences() = default;
  Preferences( const nlohmann::json & j );

  nlohmann::json toJSON() const;

  PkgPredicate pred() const;

  int compareInputs(
        const std::string_view idA, const FloxFlakeRef & a
      , const std::string_view idB, const FloxFlakeRef & b
      ) const;

    inline int
  compareInputs( const input_pair & a, const input_pair & b ) const
  {
    return this->compareInputs( a.first
                              , a.second->flake.lockedRef
                              , b.first
                              , b.second->flake.lockedRef
                              );
  }

    std::function<bool( const input_pair &, const input_pair & )>
  inputLessThan() const
  {
    return [&]( const input_pair & a, const input_pair & b )
    {
      return this->compareInputs( a, b ) < 0;
    };
  }
};


void from_json( const nlohmann::json & j,       Preferences & p );
void to_json(         nlohmann::json & j, const Preferences & p );


/* -------------------------------------------------------------------------- */

class FloxFlake {
  private:
    nix::ref<nix::EvalState> _state;
    FloxFlakeRef             _flakeRef;
    std::list<std::string>   _systems;
    std::list<std::string>   _prefsPrefixes;
    std::list<std::string>   _prefsStabilities;

    std::shared_ptr<nix::flake::LockedFlake> lockedFlake;

  public:
    FloxFlake(       nix::ref<nix::EvalState>    state
             ,       std::string_view            id
             ,       FloxFlakeRef             && ref
             , const std::list<std::string>   &  systems = defaultSystems
             , const Preferences              &  prefs   = {}
             );
    std::list<std::string>            getSystems()                      const;
    std::list<std::list<std::string>> getDefaultFlakeAttrPaths()        const;
    std::list<std::list<std::string>> getDefaultFlakeAttrPathPrefixes() const;
    std::list<std::list<std::string>> getFlakeAttrPathPrefixes()        const;

    std::shared_ptr<nix::flake::LockedFlake> getLockedFlake() const;
    nix::ref<nix::eval_cache::EvalCache>     openEvalCache()  const;

    /* Like `findAttrAlongPath' but without suggestions.
     * Note that each invocation opens the `EvalCache', so use sparingly. */
    Cursor      openCursor(      const std::vector<nix::Symbol> & path ) const;
    MaybeCursor maybeOpenCursor( const std::vector<nix::Symbol> & path ) const;

    /* Opens `EvalCache' once, staying open until all cursors die. */
    std::list<Cursor> getFlakePrefixCursors() const;

    std::list<std::vector<nix::Symbol>> getActualFlakeAttrPathPrefixes() const;
};


/* -------------------------------------------------------------------------- */

class ResolverState {
  private:
    std::shared_ptr<nix::Store>      _store;
    std::shared_ptr<nix::Store>      evalStore;
    std::shared_ptr<nix::EvalState>  evalState;
    std::map<std::string, FloxFlake> inputs;

  public:
    nix::ref<nix::EvalState> getStore();
    nix::ref<nix::EvalState> getEvalStore();
    nix::ref<nix::EvalState> getEvalState();
};


/* -------------------------------------------------------------------------- */

class Resolved {
  private:
    std::string  uri;
    FloxFlakeRef input;

  public:
    AttrPathGlob   path;
    nlohmann::json info;

    Resolved( const nlohmann::json & attrs );
    Resolved( const FloxFlakeRef   & input
            , const AttrPathGlob   & path
            , const nlohmann::json & info
            );

    nlohmann::json toJSON()   const;
    std::string    toString() const { return this->uri; }
};


void from_json( const nlohmann::json & j,       Resolved & p );
void to_json(         nlohmann::json & j, const Resolved & p );


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
