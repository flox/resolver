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

  PkgPredicate                       pred()    const;
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
    std::list<std::string>            getSystems()                      const;
    std::list<std::list<std::string>> getDefaultFlakeAttrPaths()        const;
    std::list<std::list<std::string>> getDefaultFlakeAttrPathPrefixes() const;
    std::list<std::list<std::string>> getFlakeAttrPathPrefixes()        const;

    std::shared_ptr<nix::flake::LockedFlake> getLockedFlake();
    nix::ref<nix::eval_cache::EvalCache>     openEvalCache();

    /* Like `findAttrAlongPath' but without suggestions.
     * Note that each invocation opens the `EvalCache', so use sparingly. */
    Cursor      openCursor(      const std::vector<nix::Symbol> & path );
    MaybeCursor maybeOpenCursor( const std::vector<nix::Symbol> & path );

    /* Opens `EvalCache' once, staying open until all cursors die. */
    std::list<Cursor> getFlakePrefixCursors();

    std::list<std::vector<nix::Symbol>> getActualFlakeAttrPathPrefixes();

    struct Iterator {
      private:
        Cursor                   _root;
        std::vector<nix::Symbol> _attrs;
        size_t                   _i      = 0;

      public:
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = nix::eval_cache::AttrCursor;
        using pointer           = Cursor;
        using reference         = Cursor;

        Iterator( Cursor root ) :
          _root( root ), _attrs( root->getAttrs() ), _i( 0 )
        {}
        Iterator( Cursor root, size_t i ) :
          _root( root ), _attrs( root->getAttrs() ), _i( i )
        {
          if ( this->_attrs.size() < this->_i )
            {
              this->_i = this->_attrs.size();
            }
        }

        size_t getIdx() const { return this->_i; }
        size_t getMax() const { return this->_attrs.size(); }

          reference
        operator*() const
        {
          if ( this->_attrs.size() <= this->_i )
            {
              return this->_root->getAttr(
                this->_attrs[this->_attrs.size() - 1]
              );
            }
          return this->_root->getAttr( this->_attrs[this->_i] );
        }
          pointer
        operator->() const
        {
          if ( this->_attrs.size() <= this->_i )
            {
              return this->_root->getAttr(
                this->_attrs[this->_attrs.size() - 1]
              );
            }
          return this->_root->getAttr( this->_attrs[this->_i] );
        }

        Iterator & operator++() { this->_i++; return * this; }

          Iterator
        operator++( int )
        {
          Iterator tmp = * this;
          ++( * this );
          return tmp;
        }

          friend bool
        operator==( const Iterator & a, const Iterator & b )
        {
          return ( a._i == b._i ) && ( a._attrs == b._attrs );
        }

          friend bool
        operator!=( const Iterator & a, const Iterator & b )
        {
          return ( a._i != b._i ) || ( a._attrs != b._attrs );
        }
    };

      Iterator
    beginAt( const std::vector<nix::Symbol> & path )
    {
      return Iterator( this->openCursor( path ) );
    }
    Iterator beginAt( Cursor root ) { return Iterator( root ); }

      Iterator
    endAt( const std::vector<nix::Symbol> & path )
    {
      Cursor r = this->openCursor( path );
      return Iterator( r, r->getAttrs().size() );
    }
      Iterator
    endAt( Cursor root )
    {
      return Iterator( root, root->getAttrs().size() );
    }
};


/* -------------------------------------------------------------------------- */

class ResolverState {
  private:
    std::shared_ptr<nix::Store>                       _store;
    std::shared_ptr<nix::Store>                       evalStore;
    std::shared_ptr<nix::EvalState>                   evalState;
    std::map<std::string, std::shared_ptr<FloxFlake>> _inputs;
    const Preferences                                 _prefs;

  public:
    nix::ref<nix::Store>     getStore();
    nix::ref<nix::Store>     getEvalStore();
    nix::ref<nix::EvalState> getEvalState();

    ResolverState( const Inputs                 & inputs
                 , const Preferences            & prefs
                 , const std::list<std::string> & systems = defaultSystems
                 )
      : _prefs( prefs )
    {
      for ( auto & [id, ref] : inputs.inputs )
        {
#if HAVE_BOEHMGC
          this->_inputs.emplace( id, std::allocate_shared<FloxFlake>(
            traceable_allocator<FloxFlake>()
          , this->getEvalState()
          , id
          , ref
          , this->_prefs
          , systems
          ) );
#else
          this->_inputs.emplace( id, std::make_shared<FloxFlake>(
            this->getEvalState()
          , id
          , ref
          , this->_prefs
          , systems
          ) );
#endif
        }
    }

    Preferences getPreferences() const { return this->_prefs; }

    std::map<std::string, nix::ref<FloxFlake>> getInputs() const;
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
