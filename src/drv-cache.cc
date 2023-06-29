/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <string>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <nix/eval-cache.hh>
#include <nix/sqlite.hh>
#include <nix/store-api.hh>
#include <nix/sync.hh>
#include "flox/package.hh"
#include "flox/drv-cache.hh"
#include "resolve.hh"
#include <filesystem>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  std::string_view
progressStatusToString( const progress_status & ps )
{
  switch( ps )
    {
      case DBPS_NONE:       return "NONE";       break;
      case DBPS_PARTIAL:    return "PARTIAL";    break;
      case DBPS_PATHS_DONE: return "PATHS_DONE"; break;
      case DBPS_INFO_DONE:  return "INFO_DONE";  break;
      case DBPS_EMPTY:      return "EMPTY";      break;
      case DBPS_MISSING:    return "MISSING";    break;
      case DBPS_FORCE:      return "FORCE";      break;
      default:              return "ERROR";      break;
    }
}


/* -------------------------------------------------------------------------- */

  std::string
getDrvDbName( const nix::flake::Fingerprint & fingerprint )
{
  nix::Path cacheDir = nix::getCacheDir() + "/flox/drv-cache-v0";
  std::string fpStr  = fingerprint.to_string( nix::Base16, false );
  nix::Path dbPath   = cacheDir + "/" + fpStr + ".sqlite";
  return dbPath;
}


/* -------------------------------------------------------------------------- */

static const char * schema = R"sql(
CREATE TABLE IF NOT EXISTS Derivations (
  subtree  TEXT  NOT NULL
, system   TEXT  NOT NULL
, path     JSON  NOT NULL
, PRIMARY  KEY ( subtree, system, path )
);

CREATE VIEW IF NOT EXISTS v_DerivationsAbs AS SELECT
  subtree, system, path AS relPath
, ( '["'||subtree||'","'||system||'",'||ltrim( path, '[' ) ) AS absPath
FROM Derivations;

CREATE TABLE IF NOT EXISTS DerivationInfos (
  subtree  TEXT  NOT NULL
, system   TEXT  NOT NULL
, path     JSON  NOT NULL

, fullName          TEXT  NOT NULL
, pname             TEXT  NOT NULL
, version           TEXT
, semver            TEXT
, license           TEXT
, outputs           JSON  NOT NULL DEFAULT '[]'
, outputsToInstall  JSON  NOT NULL DEFAULT '["out"]'
, broken            BOOL
, unfree            BOOL
, hasMetaAttr       BOOL  NOT NULL
, hasPnameAttr      BOOL  NOT NULL
, hasVersionAttr    BOOL  NOT NULL

, PRIMARY  KEY ( subtree, system, path )
);

CREATE TABLE IF NOT EXISTS Progress (
  subtree  TEXT     NOT NULL
, system   TEXT     NOT NULL
, status   INTEGER  NOT NULL DEFAULT 0
, PRIMARY  KEY ( subtree, system )
);

CREATE TABLE IF NOT EXISTS VersionInfo (
  id       TEXT  PRIMARY KEY
, version  TEXT  NOT NULL
);
)sql";


/* -------------------------------------------------------------------------- */


static const char * setVersionInfo =
  "INSERT OR IGNORE INTO VersionInfo ( id, version ) VALUES"
  "  ( 'resolver', '" LIBFLOX_RESOLVE_VERSION "' )"
  ", ( 'drvCacheSchema', '" FLOX_DRVDB_SCHEMA_VERSION "' )"
;


/* -------------------------------------------------------------------------- */

  static void
requireWritable( const DrvDb & cache, std::string_view name )
{
  if ( ! cache.isWritable() )
    {
      std::string msg = "DrvDb::";
      msg += name;
      msg += " requires write access to the database, but this instance was"
             " opened as read-only.";
      throw CacheException( msg );
    }
}



/* -------------------------------------------------------------------------- */

/* Populate statement templates. */
  static inline void
initStatements( nix::Sync<DrvDb::State>::Lock & state )
{
  /* Inserts */

  state->insertDrv.create(
    state->db
  , "INSERT OR REPLACE INTO Derivations ( subtree, system, path ) VALUES "
    "( ?, ?, ? )"
  );

  state->insertDrvInfo.create(
    state->db
  , "INSERT OR REPLACE INTO DerivationInfos ("
    "  subtree, system, path"
    ", fullName, pname, version, semver, license, outputs, outputsToInstall"
    ", broken, unfree, hasMetaAttr, hasPnameAttr, hasVersionAttr"
    ") VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? )"
  );

  state->insertProgress.create(
    state->db
  , "INSERT OR REPLACE INTO Progress ( subtree, system, status ) VALUES "
    "( ?, ?, ? )"
  );

  /* Queries */

  state->queryVersionInfo.create(
    state->db
  , "SELECT version FROM VersionInfo WHERE ( id = ? )"
  );


  state->hasDrv.create(
    state->db
  , "SELECT COUNT( * ) FROM Derivations "
    "WHERE ( subtree = ? ) AND ( system = ? ) AND ( path = ? )"
  );

  state->queryDrvs.create(
    state->db
  , "SELECT * FROM Derivations WHERE ( subtree = ? ) AND ( system = ? )"
  );

  state->countDrvs.create(
    state->db
  , "SELECT COUNT( subtree ) FROM Derivations WHERE"
    "( subtree = ? ) AND ( system = ? )"
  );

  state->countDrvsStability.create(
    state->db
  , "SELECT COUNT( subtree ) FROM Derivations WHERE"
    "( subtree = ? ) AND ( system = ? ) AND "
    "( json_extract( path, '$[0]' ) = ? )"
  );


  state->queryDrvInfo.create(
    state->db
  , "SELECT * FROM DerivationInfos "
    "WHERE ( subtree = ? ) AND ( system = ? ) AND ( path = ? )"
  );

  state->queryDrvInfos.create(
    state->db
  , "SELECT * FROM DerivationInfos WHERE ( subtree = ? ) AND ( system = ? )"
  );

  state->countDrvInfos.create(
    state->db
  , "SELECT COUNT( subtree ) FROM DerivationInfos WHERE"
    "( subtree = ? ) AND ( system = ? )"
  );

  state->countDrvInfosStability.create(
    state->db
  , "SELECT COUNT( subtree ) FROM DerivationInfos WHERE"
    "( subtree = ? ) AND ( system = ? ) AND "
    "( json_extract( path, '$[0]' ) = ? )"
  );


  state->queryProgress.create(
    state->db
  , "SELECT status FROM Progress WHERE ( subtree = ? ) AND ( system = ? )"
  );

  state->queryProgresses.create( state->db, "SELECT * FROM Progress" );

}


/* -------------------------------------------------------------------------- */

  static inline void
auditVersions( nix::Sync<DrvDb::State>::Lock & state )
{
  auto query1 = state->queryVersionInfo.use()( "resolver" );
  if ( ! query1.next() )
    {
      throw CacheException(
        "DrvDb(): Failed to read resolver version from from database"
      );
    }
  if ( query1.getStr( 0 ) != LIBFLOX_RESOLVE_VERSION )
    {
      throw CacheException(
        "DrvDb(): Resolver version mismatch. have: " + query1.getStr( 0 ) +
        ", want: " LIBFLOX_RESOLVE_VERSION
      );
    }

  auto query2 = state->queryVersionInfo.use()( "drvCacheSchema" );
  if ( ! query2.next() )
    {
      throw CacheException(
        "DrvDb(): Failed to read 'drvCacheSchema' from database"
      );
    }
  if ( query2.getStr( 0 ) != FLOX_DRVDB_SCHEMA_VERSION )
    {
      throw CacheException(
        "DrvDb(): Schema version mismatch. have: " + query2.getStr( 0 ) +
        ", want: " FLOX_DRVDB_SCHEMA_VERSION
      );
    }
}


/* -------------------------------------------------------------------------- */

  static void
createDb( bool recreate, bool trace, std::string_view path )
{
  std::filesystem::path p( path );
  if ( ( ! recreate ) && std::filesystem::exists( p ) ) { return; }

  if ( ! std::filesystem::exists( p.parent_path() ) )
    {
      std::filesystem::create_directories( p.parent_path() );
    }

  if ( std::filesystem::exists( p ) ) { std::filesystem::remove( p ); }

  sqlite::SQLiteDb db( std::string( path ), true, true, true, trace );
  db.exec( schema );
  db.exec( setVersionInfo );
}


/* -------------------------------------------------------------------------- */

DrvDb::DrvDb( const nix::flake::Fingerprint & fingerprint
            ,       bool                      create
            ,       bool                      write
            ,       bool                      trace
            )
  : _state( std::make_unique<nix::Sync<State>>() )
  , _write( write )
  , fingerprint( fingerprint )
{
  std::string fpStr = this->fingerprint.to_string( nix::Base16, false );
  std::string path  = getDrvDbName( this->fingerprint );

  auto state( _state->lock() );

  /* Boilerplate DB init. */
  if ( create ) { createDb( false, trace, path ); }
  state->db = sqlite::SQLiteDb(
    path
  , create
  , this->_write
  , true    /* cache */
  , trace
  );

  /* Populate statement templates, and audit existing version and schema info on
   * the off chance that there's already a DB with this fingerprint. */
  initStatements( state );

  /* Assert that the database's schema versions are good.
   * If they arent' recreate a fresh DB if able. */
  try
    {
      auditVersions( state );
    }
  catch( const CacheException & e )
    {
      if ( create )
        {
          try
            {
              if ( sqlite3_close( state->db.db ) != SQLITE_OK )
                {
                  nix::SQLiteError::throw_( state->db.db, "closing database" );
                }
              state->db.db = nullptr;
            }
          catch( ... )
            {
              nix::ignoreException();
            }
          createDb( true, trace, path );
          sqlite::SQLiteDb reopen( path, false, write, true, trace );
          state->db.db = reopen.db;
          reopen.db    = nullptr;
          auditVersions( state );
        }
      else
        {
          throw e;
        }
    }
}


/* -------------------------------------------------------------------------- */

  nix::Sync<DrvDb::State>::Lock
DrvDb::getDbState()
{
  return this->_state->lock();
}


/* -------------------------------------------------------------------------- */

  void
DrvDb::startCommit()
{
  auto state( this->getDbState() );
  /* Close existing commit if one is open. */
  if ( state->txn != nullptr )
    {
      if ( ! failed ) { state->txn->commit(); }
      state->txn.reset();
    }
  state->txn = std::make_unique<nix::SQLiteTxn>( state->db );
}

  void
DrvDb::endCommit()
{
  auto state( this->getDbState() );
  if ( state->txn != nullptr )
    {
      if ( ! failed ) { state->txn->commit(); }
      state->txn.reset();
      assert( state->txn == nullptr );
    }
}


/* -------------------------------------------------------------------------- */

DrvDb::~DrvDb()
{
  try { endCommit(); } catch ( ... ) { nix::ignoreException(); }
}


/* -------------------------------------------------------------------------- */

/* Error catching wrapper for insert statements. */
template<typename F>
  uint64_t
DrvDb::doSQLite( F && fun )
{
  if ( this->failed ) { return 0; }
  try
    {
      return fun();
    }
  catch ( nix::SQLiteError & )
    {
      nix::ignoreException();
      this->failed = true;
      return 0;
    }
}


/* -------------------------------------------------------------------------- */

/* Records that a derivation exists at an attrpath.
 * Nothing fancy, this is the simplest type of record. */
  void
DrvDb::setDrv(       std::string_view           subtree
             ,       std::string_view           system
             , const std::vector<std::string> & path
             )
{
  requireWritable( * this, "setDrv" );
  nlohmann::json relPath = path;
  this->doSQLite( [&]()
  {
    auto state( this->getDbState() );
    state->insertDrv.use()( subtree )( system )( relPath.dump() ).exec();
    uint64_t rowId = state->db.getLastInsertedRowId();
    assert( rowId != 0 );
    return rowId;
  } );
}


/* -------------------------------------------------------------------------- */

/* Convenience wrapper for reporting that a derivation exists at an attrpath.
 * This extracts that info from a `Package' record since in most routines that's
 * what we have on hand. */
  void
DrvDb::setDrv( const Package & p )
{
  requireWritable( * this, "setDrv" );
  std::vector<std::string> path    = p.getPathStrs();
  nlohmann::json           relPath = nlohmann::json::array();
  for ( size_t i = 2; i < path.size(); ++i )
    {
      relPath.push_back( path[i] );
    }
  doSQLite( [&]() {
    auto state( this->getDbState() );
    state->insertDrv.use()( path[0] )( path[1] )( relPath.dump() ).exec();
    uint64_t rowId = state->db.getLastInsertedRowId();
    assert( rowId != 0 );
    return rowId;
  } );
}


/* -------------------------------------------------------------------------- */

/* Check if an attrpath is avilable in a flake.
 * Returns `nullopt' if we haven't indicated that a flake has been completely
 * processed; but can still return `true' when partially populated. */
  std::optional<bool>
DrvDb::hasDrv(       std::string_view           subtree
             ,       std::string_view           system
             , const std::vector<std::string> & path
             )
{
  progress_status s = this->getProgress( subtree, system );
  if ( s < DBPS_PARTIAL ) { return std::nullopt; }
  nlohmann::json relPath  = path;
  std::optional<bool> rsl = std::nullopt;
  this->doSQLite( [&]() {
    auto state( this->getDbState() );
    nix::SQLiteStmt::Use query =
      state->hasDrv.use()( subtree )( system )( relPath.dump() );
    assert( query.next() );
    if ( query.getInt( 0 ) != 0 )    { rsl = true; }
    else if ( DBPS_PATHS_DONE <= s ) { rsl = false; }
    return 0;
  } );
  return rsl;  /* Inconclusive */
}


/* -------------------------------------------------------------------------- */

/* Dump all paths to derivations.
 * Returns `nullopt' if the flake isn't completely processed. */
  std::optional<std::list<std::vector<std::string>>>
DrvDb::getDrvPaths( std::string_view subtree, std::string_view system )
{
  if ( this->getProgress( subtree, system ) < DBPS_PATHS_DONE )
    {
      return std::nullopt;
    }
  std::list<std::vector<std::string>> rsl;
  this->doSQLite( [&]() {
    auto state( this->getDbState() );
    auto query = state->queryDrvs.use()( subtree )( system );
    while ( query.next() )
      {
        rsl.push_back( nlohmann::json::parse( query.getStr( 2 ) ) );
      }
    return 0;
  } );
  return rsl;
}


/* -------------------------------------------------------------------------- */

  void
DrvDb::setDrvInfo( const Package & p )
{
  requireWritable( * this, "setDrvInfo" );
  std::vector<std::string> path    = p.getPathStrs();
  nlohmann::json           relPath = nlohmann::json::array();
  for ( size_t i = 2; i < path.size(); ++i )
    {
      relPath.push_back( path[i] );
    }

  nlohmann::json outputs          = p.getOutputs();
  nlohmann::json outputsToInstall = p.getOutputsToInstall();

  this->doSQLite( [&]() {
    auto state( this->getDbState() );
    state->insertDrv.use()( path[0] )( path[1] )( relPath.dump() ).exec();
    uint64_t rowId = state->db.getLastInsertedRowId();
    assert( rowId != 0 );

    state->insertDrvInfo.use()
      ( path[0] )
      ( path[1] )
      ( relPath.dump() )
      ( p.getFullName() )
      ( p.getPname() )
      ( p.getVersion().value_or( "" ), p.getVersion().has_value() )
      ( p.getSemver().value_or( "" ), p.getSemver().has_value() )
      ( p.getLicense().value_or( "" ), p.getLicense().has_value() )
      ( outputs.dump())(outputsToInstall.dump() )
      ( p.isBroken().value_or( false ), p.isBroken().has_value() )
      ( p.isUnfree().value_or( false ), p.isUnfree().has_value() )
      ( p.hasMetaAttr() )
      ( p.hasPnameAttr() )
      ( p.hasVersionAttr() )
      .exec();
    rowId = state->db.getLastInsertedRowId();
    assert( rowId != 0 );

    return rowId;
  } );
}


/* -------------------------------------------------------------------------- */

  static nlohmann::json
infoFromQuery( nix::SQLiteStmt::Use & query )
{
  nlohmann::json info = {
    { "subtree",          query.getStr( 0 ) }
  , { "system",           query.getStr( 1 ) }
  , { "path",             nlohmann::json::parse( query.getStr( 2 ) ) }
  , { "name",             query.getStr( 3 ) }
  , { "pname",            query.getStr( 4 ) }
  , { "outputs",          nlohmann::json::parse( query.getStr( 8 ) ) }
  , { "outputsToInstall", nlohmann::json::parse( query.getStr( 9 ) ) }
  };

  if ( query.isNull( 5 ) )
    {
      info.emplace( "version", nlohmann::json() );
    }
  else
    {
      info.emplace( "version", query.getStr( 5 ) );
    }

  if ( query.isNull( 6 ) )
    {
      info.emplace( "semver", nlohmann::json() );
    }
  else
    {
      info.emplace( "semver", query.getStr( 6 ) );
    }

  if ( query.isNull( 7 ) )
    {
      info.emplace( "license", nlohmann::json() );
    }
  else
    {
      info.emplace( "license", query.getStr( 7 ) );
    }

  if ( query.isNull( 10 ) )
    {
      info.emplace( "broken", nlohmann::json() );
    }
  else
    {
      info.emplace( "broken", ( query.getInt( 10 ) != 0 ) );
    }

  if ( query.isNull( 11 ) )
    {
      info.emplace( "unfree", nlohmann::json() );
    }
  else
    {
      info.emplace( "unfree", ( query.getInt( 11 ) != 0 ) );
    }

  info.emplace( "hasMetaAttr",    ( query.getInt( 12 ) != 0 ) );
  info.emplace( "hasPnameAttr",   ( query.getInt( 13 ) != 0 ) );
  info.emplace( "hasVersionAttr", ( query.getInt( 14 ) != 0 ) );

  return info;
}


/* -------------------------------------------------------------------------- */

  std::optional<nlohmann::json>
DrvDb::getDrvInfo(       std::string_view           subtree
                 ,       std::string_view           system
                 , const std::vector<std::string> & path
                 )
{
  nlohmann::json relPath            = path;
  std::optional<nlohmann::json> rsl = std::nullopt;
  this->doSQLite( [&]() {
    auto state( this->getDbState() );
    nix::SQLiteStmt::Use query =
      state->queryDrvInfo.use()( subtree )( system )( relPath.dump() );
    if ( ! query.next() ) { rsl = std::nullopt;           }
    else                  { rsl = infoFromQuery( query ); }
    return 0;
  } );
  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::list<nlohmann::json>
DrvDb::getDrvInfos( std::string_view subtree, std::string_view system )
{
  std::list<nlohmann::json> rsl;
  this->doSQLite( [&]() {
    auto state( this->getDbState() );
    auto query = state->queryDrvInfos.use()( subtree )( system );
    while ( query.next() ) { rsl.push_back( infoFromQuery( query ) ); }
    return 0;
  } );
  return rsl;
}


/* -------------------------------------------------------------------------- */

  progress_status
DrvDb::getProgress( std::string_view subtree, std::string_view system )
{
  return (progress_status) this->doSQLite( [&]() {
    auto state( this->getDbState() );
    auto query = state->queryProgress.use()( subtree )( system );
    if ( ! query.next() ) { return (uint64_t) DBPS_NONE; }
    return (uint64_t) query.getInt( 0 );
  } );
}


/* -------------------------------------------------------------------------- */

  progress_status
DrvDb::setProgress( std::string_view subtree
                  , std::string_view system
                  , progress_status  status
                  )
{
  if ( status == DBPS_FORCE ) { return DBPS_FORCE; }
  requireWritable( * this, "setProgress" );
  progress_status old = DBPS_NONE;
  this->doSQLite( [&]() {
    auto state( this->getDbState() );
    auto query = state->queryProgress.use()( subtree )( system );
    if ( query.next() ) { old = (progress_status) query.getInt( 0 ); }
    state->insertProgress.use()( subtree )( system )( (int) status ).exec();
    uint64_t rowId = state->db.getLastInsertedRowId();
    assert( rowId != 0 );
    return rowId;
  } );
  return old;
}


/* -------------------------------------------------------------------------- */

  progress_status
DrvDb::promoteProgress( std::string_view subtree
                      , std::string_view system
                      , progress_status  status
                      )
{
  if ( status == DBPS_FORCE ) { return DBPS_FORCE; }
  requireWritable( * this, "setProgress" );
  progress_status old = DBPS_NONE;
  this->doSQLite( [&]() {
    auto state( this->getDbState() );
    auto query = state->queryProgress.use()( subtree )( system );
    if ( query.next() ) { old = (progress_status) query.getInt( 0 ); }
    if ( status <= old ) { return (uint64_t) 0; }
    state->insertProgress.use()( subtree )( system )( (int) status ).exec();
    uint64_t rowId = state->db.getLastInsertedRowId();
    assert( rowId != 0 );
    return rowId;
  } );
  return old;
}


/* -------------------------------------------------------------------------- */

CachedPackage::CachedPackage(       DrvDb                    & db
                            ,       std::string_view           subtree
                            ,       std::string_view           system
                            , const std::vector<std::string> & path
                            )
{
  std::optional _info = db.getDrvInfo( subtree, system, path );
  if ( ! _info.has_value() )
    {
      std::string p = std::string( subtree );
      p += ".";
      p += system;
      p += ".";
      for ( size_t i = 0; i < path.size(); ++i )
        {
          p += path[i];
          if ( ( i + 1 ) < path.size() )
            {
              p += ".";
            }
        }
      throw ResolverException( "CachedPackage(): No such path '" + p + "'." );
    }
  nlohmann::json info = _info.value();
  this->_pathS.push_back( info["subtree"] );
  this->_pathS.push_back( info["system"] );
  for ( auto & p : info["path"] ) { this->_pathS.push_back( p ); }
  this->_fullname = info["name"];
  this->_pname    = info["pname"];
  this->_outputs  = info["outputs"];

  this->_outputsToInstall = info["outputsToInstall"];

  if ( ! info["version"].is_null() ) { this->_version = info["version"]; }
  if ( ! info["semver"].is_null() )  { this->_semver  = info["semver"];  }
  if ( ! info["license"].is_null() ) { this->_license = info["license"]; }
  if ( ! info["broken"].is_null() )  { this->_broken  = info["broken"];  }
  if ( ! info["unfree"].is_null() )  { this->_unfree  = info["unfree"];  }

  this->_hasMetaAttr    = info["hasMetaAttr"];
  this->_hasPnameAttr   = info["hasPnameAttr"];
  this->_hasVersionAttr = info["hasVersionAttr"];
}


/* -------------------------------------------------------------------------- */

CachedPackage::CachedPackage( const nlohmann::json & drvInfo )
  : _pathS()
  , _fullname( drvInfo.at( "name" ) )
  , _pname( drvInfo.at( "pname" ) )
  , _outputs( drvInfo.at( "outputs" ) )
  , _outputsToInstall( drvInfo.at( "outputsToInstall" ) )
  , _version( drvInfo.at( "version" ).is_null()
              ? std::nullopt
              : std::make_optional( drvInfo.at( "version" ).get<std::string>() )
            )
  , _semver( drvInfo.at( "semver" ).is_null()
              ? std::nullopt
              : std::make_optional( drvInfo.at( "semver" ).get<std::string>() )
            )
  , _license( drvInfo.at( "license" ).is_null()
              ? std::nullopt
              : std::make_optional( drvInfo.at( "license" ).get<std::string>() )
            )
  , _broken( drvInfo.at( "broken" ).is_null()
              ? std::nullopt
              : std::make_optional( drvInfo.at( "broken" ).get<bool>() )
            )
  , _unfree( drvInfo.at( "unfree" ).is_null()
              ? std::nullopt
              : std::make_optional( drvInfo.at( "unfree" ).get<bool>() )
            )
  , _hasMetaAttr( drvInfo.at( "hasMetaAttr" ).get<bool>() )
  , _hasPnameAttr( drvInfo.at( "hasPnameAttr" ).get<bool>() )
  , _hasVersionAttr( drvInfo.at( "hasVersionAttr" ).get<bool>() )
{
  this->_pathS.push_back( drvInfo["subtree"] );
  this->_pathS.push_back( drvInfo["system"] );
  for ( auto & p : drvInfo["path"] ) { this->_pathS.push_back( p ); }
}


/* -------------------------------------------------------------------------- */

  std::unordered_map<std::string
                    , std::unordered_map<std::string, progress_status>
                    >
DrvDb::getProgresses()
{
  std::unordered_map<std::string
                    , std::unordered_map<std::string
                                        , progress_status
                                        >
                    >                                      rsl;
  auto insert =
    [&]( std::string     && subtree
       , std::string     && system
       , progress_status    s
       )
    {
      std::unordered_map<std::string, progress_status> e;
      e.emplace( system, s );
      auto [prev, created] = rsl.try_emplace( subtree, e );
      if ( ! created )
        {
          prev->second.insert( std::move( e.extract( e.begin() ) ) );
        }
    };
  this->doSQLite( [&]() {
    auto state( this->getDbState() );
    auto query = state->queryProgresses.use();
    while ( query.next() )
      {
        insert( query.getStr( 0 )
              , query.getStr( 1 )
              , (progress_status) query.getInt( 2 )
              );
      }
    return 0;
  } );
  return rsl;
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
