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

/**
 * A queue of cursors used to stash sub-attrsets that need to be searched
 * recursively in various iterators.
 */
using todo_queue = std::queue<Cursor, std::list<Cursor>>;

/**
 * A function type that can be "mapped" ( applied to elements by an iterator )
 * to attribute set cursors.
 */
using cursor_op = std::function<void(
        subtree_type               subtreeType
,       std::string_view           system
,       todo_queue               & todos
, const std::vector<std::string> & parentRelPath
,       std::string_view           attrName
,       Cursor                     cur
)>;

/**
 * A function type that can be "mapped" ( applied to elements by an iterator )
 * to derivations in an attribute set.
 */
using derivation_op = std::function<void(
        DrvDb                    & db
,       subtree_type               subtreeType
,       std::string_view           subtree
,       std::string_view           system
, const std::vector<std::string> & parentRelPath
,       std::string_view           attrName
,       Cursor                     cur
)>;

/**
 * A simple `cursor_op' function that pushes cursor positions for attrsets which
 * contain a `recurseIntoAttrs = true;' indicator.
 *
 * This is a sane default function for recursively traversing `legacyPackages'
 * style subtrees.
 */
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

/**
 * A convenience wrapper that provides various operations on a `flake'.
 *
 * Notably this class is responsible for owning both the `nix' `EvalState',
 * `EvalCache' database, and our extended `DrvDb' database associated with
 * a `flake'.
 *
 * It is recommended that only one `FloxFlake' be created for a unique `flake'
 * to avoid synchronization slowdowns with its databases.
 */
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

    FloxFlakeRef getFlakeRef() const { return this->_flakeRef; }

      FloxFlakeRef
    getLockedFlakeRef()
    {
      return this->getLockedFlake()->flake.lockedRef;
    }

    std::list<std::string> getSystems() const { return this->_systems; }

    std::list<std::list<std::string>> getFlakeAttrPathPrefixes() const;

    /**
     * Like `findAttrAlongPath' but without suggestions.
     * Note that each invocation opens the `EvalCache', so use sparingly.
     */
    Cursor      openCursor(      const std::vector<nix::Symbol> & path );
    MaybeCursor maybeOpenCursor( const std::vector<nix::Symbol> & path );

    /** Opens `EvalCache' once, staying open until all cursors die. */
    std::list<Cursor> getFlakePrefixCursors();

};


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
