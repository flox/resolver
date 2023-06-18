/* ========================================================================== *
 *
 * This is largely borrowed from `<nix>/src/libcmd/commands.hh' except we drop
 * `run' member functions, parsers, and some other unnecessary portions.
 *
 * -------------------------------------------------------------------------- */

#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include <optional>
#include <vector>
#include <map>
#include <memory>
#include "flox/types.hh"
#include "flox/util.hh"
#include "flox/drv-cache.hh"
#include <queue>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

FloxFlake::FloxFlake(       nix::ref<nix::EvalState>   state
                    ,       std::string_view           id
                    , const FloxFlakeRef             & ref
                    , const Preferences              & prefs
                    , const std::list<std::string>   & systems
                    )
  : _state( state )
  , _flakeRef( nix::FlakeRef::fromAttrs( ref.toAttrs() ) )
  , _systems( systems )
  , _prefsStabilities(
      ( prefs.stabilities.find( std::string( id ) ) != prefs.stabilities.end() )
      ? prefs.stabilities.at( std::string( id ) )
      : defaultCatalogStabilities
    )
  , _prefsPrefixes(
      ( prefs.prefixes.find( std::string( id ) ) != prefs.prefixes.end() )
      ? prefs.prefixes.at( std::string( id ) )
      : defaultAttrPathPrefixes
    )
{
}


/* -------------------------------------------------------------------------- */

  std::shared_ptr<nix::flake::LockedFlake>
FloxFlake::getLockedFlake()
{
  if ( this->lockedFlake == nullptr )
    {
      bool oldPurity = nix::evalSettings.pureEval;
      nix::evalSettings.pureEval = false;
      this->lockedFlake = std::make_shared<nix::flake::LockedFlake>(
        nix::flake::lockFlake( * this->_state
                             , this->_flakeRef
                             , floxFlakeLockFlags
                             )
      );
      nix::evalSettings.pureEval = oldPurity;
  }
  return this->lockedFlake;
}


/* -------------------------------------------------------------------------- */

  std::list<std::string>
FloxFlake::getSystems() const
{
  return this->_systems;
}


/* -------------------------------------------------------------------------- */

  std::list<std::list<std::string>>
FloxFlake::getDefaultFlakeAttrPathPrefixes() const
{
  std::string system = nix::settings.thisSystem.get();
  if ( ! hasElement( this->_systems, system ) )
    {
      return {};
    }
  std::list<std::list<std::string>> rsl;
  for ( auto & prefix : this->_prefsPrefixes )
    {
      if ( prefix == "catalog" )
        {
          for ( auto & stability : this->_prefsStabilities )
            {
              rsl.push_back( { "catalog", system, stability } );
            }
        }
      else
        {
          rsl.push_back( { prefix, system } );
        }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::list<std::list<std::string>>
FloxFlake::getFlakeAttrPathPrefixes() const
{
  std::list<std::list<std::string>> rsl;
  for ( auto & prefix : this->_prefsPrefixes )
    {
      for ( auto & system : this->_systems )
        {
          if ( prefix == "catalog" )
            {
              for ( auto & stability : this->_prefsStabilities )
                {
                  rsl.push_back( { "catalog", system, stability } );
                }
            }
          else
            {
              rsl.push_back( { prefix, system } );
            }
        }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

   nix::ref<nix::eval_cache::EvalCache>
FloxFlake::openEvalCache()
{
  nix::flake::Fingerprint fingerprint =
    this->getLockedFlake()->getFingerprint();
  return nix::make_ref<nix::eval_cache::EvalCache>(
    ( nix::evalSettings.useEvalCache && nix::evalSettings.pureEval )
    ? std::optional { std::cref( fingerprint ) }
    : std::nullopt
  , * this->_state
  , [&]()
    {
      nix::Value * vFlake = this->_state->allocValue();
      nix::flake::callFlake(
        * this->_state
      , * this->getLockedFlake()
      , * vFlake
      );
      this->_state->forceAttrs(
        * vFlake, nix::noPos, "while parsing cached flake data"
      );
      nix::Attr * aOutputs = vFlake->attrs->get(
        this->_state->symbols.create( "outputs" )
      );
      assert( aOutputs != nullptr );
      return aOutputs->value;
    }
  );
}


/* -------------------------------------------------------------------------- */

  Cursor
FloxFlake::openCursor( const std::vector<nix::Symbol> & path )
{
  Cursor cur = this->openEvalCache()->getRoot();
  for ( const nix::Symbol & p : path ) { cur = cur->getAttr( p ); }
  return cur;
}


/* -------------------------------------------------------------------------- */

  MaybeCursor
FloxFlake::maybeOpenCursor( const std::vector<nix::Symbol> & path )
{
  MaybeCursor cur = this->openEvalCache()->getRoot();
  for ( const nix::Symbol & p : path )
    {
      cur = cur->maybeGetAttr( p );
      if ( cur == nullptr ) { break; }
    }
  return cur;
}


/* -------------------------------------------------------------------------- */

  std::list<Cursor>
FloxFlake::getFlakePrefixCursors()
{
  std::list<Cursor>                    rsl;
  nix::ref<nix::eval_cache::EvalCache> cache = this->openEvalCache();
  for ( std::list<std::string> & prefix : this->getFlakeAttrPathPrefixes() )
    {
      MaybeCursor cur = cache->getRoot();
      for ( std::string & p : prefix )
        {
          cur = cur->maybeGetAttr( p );
          if ( cur == nullptr ) { break; }
        }
      if ( cur != nullptr ) { rsl.push_back( Cursor( cur ) ); }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::list<std::vector<std::string>>
FloxFlake::getActualFlakeAttrPathPrefixes()
{
  std::list<std::vector<std::string>> rsl;
  for ( const Cursor c : this->getFlakePrefixCursors() )
    {
      std::vector<std::string> path;
      for ( const nix::Symbol & s : c->getAttrPath() )
        {
          path.push_back( this->_state->symbols[s] );
        }
      rsl.push_back( std::move( path ) );
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  progress_status
FloxFlake::populateDerivations( std::string_view subtree
                              , std::string_view system
                              )
{
  DrvDb db( this->getLockedFlake()->getFingerprint() );
  progress_status old = db.getProgress( subtree, system );
  if ( DBPS_PATHS_DONE <= old ) { return old; }

  std::vector<nix::Symbol> rootPath = {
    this->_state->symbols.create( subtree )
  , this->_state->symbols.create( system )
  };
  MaybeCursor mc = this->maybeOpenCursor( rootPath );

  /* If there is no such prefix mark it as empty and bail. */
  if ( mc == nullptr )
    {
      db.promoteProgress( subtree, system, DBPS_EMPTY );
      return DBPS_EMPTY;
    }

  db.promoteProgress( subtree, system, DBPS_PARTIAL );

  /* For `packages' prefix we can just fill attrnames. */
  if ( subtree == "packages" )
    {
      for ( const nix::Symbol & a : mc->getAttrs() )
        {
          // TODO: Maybe ensure these are derivations?
          db.setDrv( subtree, system, { this->_state->symbols[a] } );
        }
    }
  else
    {
      std::queue<Cursor, std::list<Cursor>> todos;
      todos.push( (Cursor) mc );
      while ( ! todos.empty() )
        {
          std::vector<std::string> relPath;
          size_t pi = 0;
          for ( const nix::Symbol & s : todos.front()->getAttrPath() )
            {
              if ( pi < 2 ) { ++pi; continue; }
              relPath.push_back( this->_state->symbols[s] );
            }
          for ( const nix::Symbol s : todos.front()->getAttrs() )
            {
              try
                {
                  Cursor c = todos.front()->getAttr( s );
                  if ( c->isDerivation() )
                    {
                      /* Cache the evaluated result. */
                      relPath.push_back( this->_state->symbols[s] );
                      db.setDrv( subtree, system, relPath );
                      relPath.pop_back();
                    }
                  else if ( subtree != "packages" )
                    {
                      MaybeCursor m =
                        c->maybeGetAttr( "recurseForDerivations" );
                      if ( ( m != nullptr ) && m->getBool() )
                        {
                          todos.push( (Cursor) c );
                        }
                    }
                }
              catch( ... )
                {
                  // TODO: Catch errors in `packages'.
                }
            }
        }
    }
  db.promoteProgress( subtree, system, DBPS_PATHS_DONE );
  return DBPS_PATHS_DONE;
}



/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

