/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include "flox/types.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

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

    std::shared_ptr<nix::flake::LockedFlake> getLockedFlake();
    nix::ref<nix::eval_cache::EvalCache>     openEvalCache();

    FloxFlakeRef getFlakeRef()  const { return this->_flakeRef; }

      FloxFlakeRef
    getLockedFlakeRef()
    {
      return this->getLockedFlake()->flake.lockedRef;
    }

    std::list<std::string>            getSystems()                      const;
    std::list<std::list<std::string>> getDefaultFlakeAttrPathPrefixes() const;
    std::list<std::list<std::string>> getFlakeAttrPathPrefixes()        const;

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

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
