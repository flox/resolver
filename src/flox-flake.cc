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
#include "flox/eval-package.hh"


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

  Cursor
FloxFlake::openCursor( const std::vector<nix::SymbolStr> & path )
{
  Cursor cur = this->openEvalCache()->getRoot();
  for ( const nix::SymbolStr & p : path ) { cur = cur->getAttr( p ); }
  return cur;
}

  Cursor
FloxFlake::openCursor( const std::vector<std::string> & path )
{
  Cursor cur = this->openEvalCache()->getRoot();
  for ( const std::string & p : path ) { cur = cur->getAttr( p ); }
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

  MaybeCursor
FloxFlake::maybeOpenCursor( const std::vector<nix::SymbolStr> & path )
{
  MaybeCursor cur = this->openEvalCache()->getRoot();
  for ( const nix::SymbolStr & p : path )
    {
      cur = cur->maybeGetAttr( p );
      if ( cur == nullptr ) { break; }
    }
  return cur;
}

  MaybeCursor
FloxFlake::maybeOpenCursor( const std::vector<std::string> & path )
{
  MaybeCursor cur = this->openEvalCache()->getRoot();
  for ( const std::string & p : path )
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
FloxFlake::derivationsDo( std::string_view subtree
                        , std::string_view system
                        , progress_status  doneStatus
                        , derivation_op    drvOp
                        , cursor_op        nonDrvOp
                        )
{
  DrvDb db( this->getLockedFlake()->getFingerprint() );
  progress_status old = db.getProgress( subtree, system );
  if ( doneStatus <= old ) { return old; }

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

  subtree_type stt = parseSubtreeType( subtree );

  todo_queue todos;

  auto runDrvOp = [&](
    const std::vector<std::string> & parentRelPath
  ,       std::string_view           attrName
  ,       Cursor                     cur
  ) { drvOp( db, stt, subtree, system, parentRelPath, attrName, cur ); };
  auto runNonDrvOp = [&](
    const std::vector<std::string> & parentRelPath
  ,       std::string_view           attrName
  ,       Cursor                     cur
  ) { nonDrvOp( stt, system, todos, parentRelPath, attrName, cur ); };

  db.promoteProgress( subtree, system, DBPS_PARTIAL );

  /* For `packages' prefix we can just fill attrnames. */
  if ( stt == ST_PACKAGES )
    {
      const std::vector<std::string> parentRelPath;
      for ( const nix::Symbol & a : mc->getAttrs() )
        {
          // TODO: Maybe ensure these are derivations?
          runDrvOp( parentRelPath
                  , (std::string_view) this->_state->symbols[a]
                  , mc->getAttr( a )
                  );
        }
    }
  else
    {
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
                      runDrvOp( relPath
                              , (std::string_view) this->_state->symbols[s]
                              , c
                              );
                    }
                  else
                    {
                      runNonDrvOp( relPath
                                 , (std::string_view) this->_state->symbols[s]
                                 , c
                                 );
                    }
                }
              catch( ... )
                {
                  // TODO: Catch errors in `packages'.
                }
            }
        }
    }
  if ( doneStatus != DBPS_FORCE )
    {
      db.promoteProgress( subtree, system, doneStatus );
    }
  return doneStatus;
}



/* -------------------------------------------------------------------------- */

  progress_status
FloxFlake::populateDerivations( std::string_view subtree
                              , std::string_view system
                              )
{
  return derivationsDo( subtree, system, DBPS_PATHS_DONE, [](
          DrvDb                    & db
  ,       subtree_type               subtreeType
  ,       std::string_view           subtree
  ,       std::string_view           system
  , const std::vector<std::string> & parentRelPath
  ,       std::string_view           attrName
  ,       Cursor                     cur
  )
  {
    std::vector<std::string> path;
    if ( subtreeType == ST_PACKAGES )
      {
        path = { std::string( attrName ) };
      }
    else
      {
        path = parentRelPath;
        path.push_back( std::string( attrName ) );
      }
    db.setDrv( subtree, system, path );
  } );
}



/* -------------------------------------------------------------------------- */

  void
FloxFlake::packagesDo(
  std::string_view                                         subtree
, std::string_view                                         system
, std::function<void(std::any * aux, const Package & p)>   op
, std::any                                               * aux
, bool                                                     allowCache
)
{
  DrvDb db( this->getLockedFlake()->getFingerprint() );
  if ( allowCache )
    {
      progress_status old = db.getProgress( subtree, system );
      if ( DBPS_INFO_DONE <= old )
        {
          for ( auto & dinfo : db.getDrvInfos( subtree, system ) )
            {
              op( aux, CachedPackage( dinfo ) );
            }
        }
      else if ( DBPS_PATHS_DONE <= old )
        {
          const std::list<std::vector<std::string>> relPaths =
            db.getDrvPaths( subtree, system ).value();
          if ( relPaths.empty() )
            {
              db.promoteProgress( subtree, system, DBPS_EMPTY );
              return;
            }
          std::vector<nix::Symbol> path = {
            this->_state->symbols.create( subtree )
          , this->_state->symbols.create( system )
          };
          Cursor prefix = this->openCursor( path );
          for ( std::vector<std::string> rel : relPaths )
            {
              Cursor c = prefix;
              for ( std::string a : rel ) { c = c->getAttr( a ); }
              EvalPackage p( c, & this->_state->symbols, false );
              /* Cache the result for next time. */
              db.setDrvInfo( p );
              /* Run our operation. */
              op( aux, p );
            }
          db.promoteProgress( subtree, system, DBPS_INFO_DONE );
          return;
        }
    }

  nix::SymbolTable * st = & this->_state->symbols;
  this->derivationsDo( subtree, system, DBPS_FORCE, [&](
          DrvDb                    & db
  ,       subtree_type               subtreeType
  ,       std::string_view           subtree
  ,       std::string_view           system
  , const std::vector<std::string> & parentRelPath
  ,       std::string_view           attrName
  ,       Cursor                     cur
  ) {
    EvalPackage p( cur, st, false );
    /* Cache the result for next time. */
    if ( allowCache ) { db.setDrvInfo( p ); }
    /* Run our operation. */
    op( aux, p );
  } );
  if ( allowCache ) { db.promoteProgress( subtree, system, DBPS_INFO_DONE ); }
}


/* -------------------------------------------------------------------------- */

// TODO: Use DrvDb
  std::list<Cursor>
FloxFlake::openCursorsByAttrPathGlob( const AttrPathGlob & path )
{

  std::list<Cursor> rsl;

  if ( path.isAbsolute() )
    {
      MaybeCursor prefix = this->openEvalCache()->getRoot()->maybeGetAttr(
        std::get<std::string>( path.path[0] )
      );
      if ( prefix == nullptr ) { return {}; }
      if ( ! path.hasGlob() )
        {
          MaybeCursor c = prefix;
          for ( size_t i = 1; ( i < path.size() ) && ( c != nullptr ); ++i )
            {
              c = c->maybeGetAttr( std::get<std::string>( path.path[i] ) );
            }
          if ( c != nullptr ) { rsl.push_back( std::move( (Cursor) c ) ); }
        }
      else
        {
          for ( const auto & system : this->_systems )
            {
              MaybeCursor c = prefix->maybeGetAttr( system );
              for ( size_t i = 2; ( i < path.size() ) && ( c != nullptr ); ++i )
                {
                  c = c->maybeGetAttr( std::get<std::string>( path.path[i] ) );
                }
              if ( c != nullptr ) { rsl.push_back( std::move( (Cursor) c ) ); }
            }
        }
    }
  else  /* Relative */
    {
      for ( const auto & prefix : this->getFlakePrefixCursors() )
        {
          MaybeCursor c = prefix;
          for ( size_t i = 0; ( i < path.size() ) && ( c != nullptr ); ++i )
            {
              c = c->maybeGetAttr( std::get<std::string>( path.path[i] ) );
            }
          if ( c != nullptr ) { rsl.push_back( std::move( (Cursor) c ) ); }
        }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */




/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

