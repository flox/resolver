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
#include "flox/exceptions.hh"
#include <queue>
#include <any>


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

using FloxFlakeRef = nix::FlakeRef;
using input_pair   =
  std::pair<std::string, std::shared_ptr<nix::flake::LockedFlake>>;

using Cursor      = nix::ref<nix::eval_cache::AttrCursor>;
using CursorPos   = std::pair<Cursor, std::vector<nix::Symbol>>;
using MaybeCursor = std::shared_ptr<nix::eval_cache::AttrCursor>;


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
  DBPS_NONE       = 0 /* Indicates that a DB is completely fresh. */
, DBPS_PARTIAL    = 1 /* Indicates some partially populated state. */
, DBPS_PATHS_DONE = 2 /* Indicates that we know all derivation paths. */
, DBPS_INFO_DONE  = 3 /* Indicates that we have collected info metadata. */
, DBPS_EMPTY      = 4 /* Indicates that a prefix has no values. */
, DBPS_MISSING    = 5 /* Indicates that a DB is completely fresh. */
, DBPS_FORCE      = 6 /* This should always have highest value. */
}  progress_status;

std::string_view progressStatusToString( const progress_status & ps );

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

class Inputs {
  private:
    std::unordered_map<std::string, FloxFlakeRef> inputs;

    void init( const nlohmann::json & j );

  public:

    Inputs() = default;
    Inputs( const nlohmann::json & j ) { this->init( j ); }

    bool           has( std::string_view id ) const;
    FloxFlakeRef   get( std::string_view id ) const;
    nlohmann::json toJSON()                   const;

    std::list<std::string_view> getInputNames() const;
};


void from_json( const nlohmann::json & j,       Inputs & i );
void to_json(         nlohmann::json & j, const Inputs & i );


/* -------------------------------------------------------------------------- */

namespace predicates { struct PkgPred; };

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

  flox::resolve::predicates::PkgPred pred_V2() const;

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

using todo_queue = std::queue<Cursor, std::list<Cursor>>;

using cursor_op = std::function<void(
        subtree_type               subtreeType
,       std::string_view           system
,       todo_queue               & todos
, const std::vector<std::string> & parentRelPath
,       std::string_view           attrName
,       Cursor                     cur
)>;

using derivation_op = std::function<void(
        DrvDb                    & db
,       subtree_type               subtreeType
,       std::string_view           subtree
,       std::string_view           system
, const std::vector<std::string> & parentRelPath
,       std::string_view           attrName
,       Cursor                     cur
)>;

  static const cursor_op
handleRecurseForDerivations = [](
        subtree_type               subtreeType
,       std::string_view           system
,       todo_queue               & todos
, const std::vector<std::string> & parentRelPath
,       std::string_view           attrName
,       Cursor                     cur
)
{
  if ( subtreeType != ST_PACKAGES )
    {
      MaybeCursor m = cur->maybeGetAttr( "recurseForDerivations" );
      if ( ( m != nullptr ) && m->getBool() ) { todos.push( (Cursor) cur ); }
    }
};


/* -------------------------------------------------------------------------- */

class FloxFlake : public std::enable_shared_from_this<FloxFlake> {
  private:
    nix::ref<nix::EvalState> _state;
    FloxFlakeRef             _flakeRef;
    std::list<std::string>   _systems;
    std::vector<std::string> _prefsPrefixes;
    std::vector<std::string> _prefsStabilities;

    std::shared_ptr<nix::flake::LockedFlake> lockedFlake;

  public:
    FloxFlake(       nix::ref<nix::EvalState>   state
             ,       std::string_view           id
             , const FloxFlakeRef             & ref
             , const Preferences              & prefs
             , const std::list<std::string>   & systems = defaultSystems
             );

    FloxFlakeRef getFlakeRef() const { return this->_flakeRef; }

    std::list<std::string>            getSystems()                      const;
    std::list<std::list<std::string>> getDefaultFlakeAttrPathPrefixes() const;
    std::list<std::list<std::string>> getFlakeAttrPathPrefixes()        const;

    std::shared_ptr<nix::flake::LockedFlake> getLockedFlake();
    nix::ref<nix::eval_cache::EvalCache>     openEvalCache();

    /**
     * Like `findAttrAlongPath' but without suggestions.
     * Note that each invocation opens the `EvalCache', so use sparingly.
     */
    Cursor      openCursor(      const std::vector<nix::Symbol>    & path );
    Cursor      openCursor(      const std::vector<nix::SymbolStr> & path );
    Cursor      openCursor(      const std::vector<std::string>    & path );
    MaybeCursor maybeOpenCursor( const std::vector<nix::Symbol>    & path );
    MaybeCursor maybeOpenCursor( const std::vector<nix::SymbolStr> & path );
    MaybeCursor maybeOpenCursor( const std::vector<std::string>    & path );

    /** Opens `EvalCache' once, staying open until all cursors die. */
    std::list<Cursor> getFlakePrefixCursors();

    std::list<std::vector<std::string>> getActualFlakeAttrPathPrefixes();

    /**
     * Try opening cursors from an absolute or relative path with globs.
     * Glob is only accepted for `system'.
     */
    std::list<Cursor> openCursorsByAttrPathGlob( const AttrPathGlob & path );

    /**
     * Determine which output prefixes exist in this flake, ignoring
     * `preferences' and mark them in the `DrvDb'.
     * This allows optimized lookups in later attempts to resolve against the
     * same flake with different preferences.
     */
    void recordPrefixes( bool force = false /** Do not optimize out eval. */ );

    /**
     * Populate this flake's `DrvDb' with paths for all derivations under the
     * given prefix.
     * This does NOT populate "info" fields, it only records paths to
     * all derivations.
     * This routine is significantly more lightweight than ones focused on
     * scraping "info" fields making it suitable for quickly resolving
     * absolute and relative path descriptors.
     */
    progress_status populateDerivations( std::string_view subtree
                                       , std::string_view system
                                       );

    /**
     * Apply a function to all derivations with access to an `AttrCursor'.
     * `nonDrvOp' allows you to control recursive descent into additional
     * subtrees by appending the `todos' argument passed to `cursor_op'.
     */
    progress_status derivationsDo(
      std::string_view subtree
    , std::string_view system
    , progress_status  doneStatus  /** Use `DBPS_FORCE' to avoid skipping. */
    , derivation_op    drvOp
    , cursor_op        nonDrvOp   = handleRecurseForDerivations
    );

    /**
     * Apply a function to all `Packages' under a prefix.
     * This abstracts lookups in caches vs. eval, and will implicitly populate
     * missing cache members unless `allowCache' disables this behavior.
     * Arbitrary auxilary data can be accessed within `op' by the `aux'
     * argument ( optional ).
     */
    void packagesDo(
      std::string_view                                         subtree
    , std::string_view                                         system
    , std::function<void( std::any * aux, const Package & p )> op

    , std::any * aux        = nullptr
    , bool       allowCache = true
    );
};


/* -------------------------------------------------------------------------- */

class Resolved {
  private:
    FloxFlakeRef input;

  public:
    AttrPathGlob   path;
    nlohmann::json info;

    Resolved( const nlohmann::json & attrs )
      : input( nix::FlakeRef::fromAttrs(
                 nix::fetchers::jsonToAttrs( attrs.at( "input" ) )
               ) )
      , path( AttrPathGlob::fromJSON( attrs.at( "path" ) ) )
      , info( attrs.at( "info" ) )
    {}

    Resolved( const FloxFlakeRef   & input
            , const AttrPathGlob   & path
            , const nlohmann::json & info
            )
      : input( input ), path( path ), info( info )
    {}

      std::string
    toString() const
    {
      return this->input.to_string() + "#" + this->path.toString();
    }

      nlohmann::json
    toJSON() const
    {
      return {
        { "input", nix::fetchers::attrsToJSON( this->input.toAttrs() ) }
      , { "uri",   this->toString() }
      , { "info",  this->info }
      , { "path",  this->path.toJSON() }
      };
    }
};


void from_json( const nlohmann::json & j,       Resolved & p );
void to_json(         nlohmann::json & j, const Resolved & p );


/* -------------------------------------------------------------------------- */

class ResolverState {
  private:
    std::shared_ptr<nix::Store>                       _store;
    std::shared_ptr<nix::Store>                       evalStore;
    std::shared_ptr<nix::EvalState>                   evalState;
    std::map<std::string, std::shared_ptr<FloxFlake>> _inputs;
    const Preferences                                 _prefs;

  public:

    nix::ref<nix::Store>       getStore();
    nix::ref<nix::Store>       getEvalStore();
    nix::ref<nix::EvalState>   getEvalState();
    nix::SymbolTable         * getSymbolTable();

    ResolverState( const Inputs                 & inputs
                 , const Preferences            & prefs
                 , const std::list<std::string> & systems = defaultSystems
                 );

    Preferences getPreferences() const { return this->_prefs; }

    std::map<std::string, nix::ref<FloxFlake>> getInputs() const;
    std::list<std::string_view>                getInputNames() const;

    std::optional<nix::ref<FloxFlake>> getInput( std::string_view id ) const;

    std::list<Resolved> resolveInInput(       std::string_view   id
                                      , const Descriptor       & desc
                                      );
};


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
