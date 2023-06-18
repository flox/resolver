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
#include "flox/package.hh"
#include "flox/drv-cache.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

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

, fullName         TEXT  NOT NULL
, pname            TEXT  NOT NULL
, version          TEXT
, semver           TEXT
, license          TEXT
, outputs          JSON  NOT NULL DEFAULT '[]'
, outputsToInstall JSON  NOT NULL DEFAULT '["out"]'
, broken           BOOL
, unfree           BOOL
, hasMetaAttr      BOOL  NOT NULL
, hasPnameAttr     BOOL  NOT NULL
, hasVersionAttr   BOOL  NOT NULL

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

INSERT OR IGNORE INTO VersionInfo ( id, version ) VALUES
  ( 'resolver',       '0.1.0' )
, ( 'drvCacheSchema', '0.1.0' )
;

CREATE TABLE IF NOT EXISTS Fingerprint ( fingerprint TEXT PRIMARY KEY );
)sql";


/* -------------------------------------------------------------------------- */

DrvDb::DrvDb( const nix::flake::Fingerprint & fingerprint )
  : _state( std::make_unique<nix::Sync<State>>() )
{
  auto state( _state->lock() );

  nix::Path cacheDir = nix::getCacheDir() + "/flox/drv-cache-v0";
  nix::createDirs( cacheDir );

  std::string fpStr = fingerprint.to_string( nix::Base16, false );

  nix::Path dbPath = cacheDir + "/" + fpStr + ".sqlite";

  state->db = nix::SQLite( dbPath );
  state->db.isCache();
  state->db.exec( schema );

  state->insertFingerprint.create(
    state->db
  , "INSERT OR IGNORE INTO Fingerprint VALUES ( ? )"
  );
  state->insertFingerprint.use()( fpStr ).exec();
  state->queryFingerprint.create(
    state->db
  , "SELECT fingerprint FROM Fingerprint LIMIT 1"
  );
  auto query0 = state->queryFingerprint.use();
  if ( ! query0.next() )
    {
      throw ResolverException(
        "DrvDb(): Failed to read fingerprint from database"
      );
    }
  if ( query0.getStr( 0 ) != fpStr )
    {
      throw ResolverException(
        "DrvDb(): Fingerprint mismatch in '" + fpStr + ".sqlite' reporting: "
        + query0.getStr( 0 )
      );
    }

  state->queryVersionInfo.create(
    state->db
  , "SELECT version FROM VersionInfo WHERE ( id = ? )"
  );
  auto query1 = state->queryVersionInfo.use()( "resolver" );
  if ( ! query1.next() )
    {
      throw ResolverException(
        "DrvDb(): Failed to read resolver version from from database"
      );
    }
  if ( query1.getStr( 0 ) != "0.1.0" )  // TODO: make a var
    {
      throw ResolverException(
        "DrvDb(): Resolver version mismatch in '" + fpStr + ".sqlite' "
        "reporting: " + query1.getStr( 0 )
      );
    }

  auto query2 = state->queryVersionInfo.use()( "drvCacheSchema" );
  if ( ! query2.next() )
    {
      throw ResolverException(
        "DrvDb(): Failed to read 'drvCacheSchema' from database"
      );
    }
  if ( query2.getStr( 0 ) != "0.1.0" )  // TODO: make a var
    {
      throw ResolverException(
        "DrvDb(): Schema version mismatch in '" + fpStr + ".sqlite' reporting: "
        + query2.getStr( 0 )
      );
    }

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

  state->queryDrvs.create(
    state->db
  , "SELECT * FROM Derivations WHERE ( subtree = ? ) AND ( system = ? )"
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

  state->queryProgress.create(
    state->db
  , "SELECT status FROM Progress WHERE ( subtree = ? ) AND ( system = ? )"
  );

  state->txn = std::make_unique<nix::SQLiteTxn>( state->db );
}


/* -------------------------------------------------------------------------- */

DrvDb::~DrvDb()
{
  try
    {
      auto state( _state->lock() );
      if ( ! failed ) { state->txn->commit(); }
      state->txn.reset();
    }
  catch ( ... )
    {
      nix::ignoreException();
    }
}


/* -------------------------------------------------------------------------- */

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

  uint64_t
DrvDb::setDrv( std::string_view                 subtree
             , std::string_view                 system
             , const std::vector<std::string> & path
             )
{
  nlohmann::json relPath = path;
  return doSQLite( [&]() {
    auto state( this->_state->lock() );
    state->insertDrv.use()( subtree )( system )( relPath.dump() ).exec();
    uint64_t rowId = state->db.getLastInsertedRowId();
    assert( rowId != 0 );
    return rowId;
  } );
}


/* -------------------------------------------------------------------------- */

  uint64_t
DrvDb::setDrv( const Package & p )
{
  std::vector<std::string> path    = p.getPathStrs();
  nlohmann::json           relPath = nlohmann::json::array();
  for ( size_t i = 2; i < path.size(); ++i )
    {
      relPath.push_back( path[i] );
    }
  return doSQLite( [&]() {
    auto state( this->_state->lock() );
    state->insertDrv.use()( path[0] )( path[1] )( relPath.dump() ).exec();
    uint64_t rowId = state->db.getLastInsertedRowId();
    assert( rowId != 0 );
    return rowId;
  } );
}


/* -------------------------------------------------------------------------- */

  uint64_t
DrvDb::setDrvInfo( const Package & p )
{
  std::vector<std::string> path    = p.getPathStrs();
  nlohmann::json           relPath = nlohmann::json::array();
  for ( size_t i = 2; i < path.size(); ++i )
    {
      relPath.push_back( path[i] );
    }

  nlohmann::json outputs          = p.getOutputs();
  nlohmann::json outputsToInstall = p.getOutputsToInstall();

  return doSQLite( [&]() {
    auto state( this->_state->lock() );
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
      info.emplace( "version", nullptr );
    }
  else
    {
      info.emplace( "version", query.getStr( 5 ) );
    }

  if ( query.isNull( 6 ) )
    {
      info.emplace( "semver", nullptr );
    }
  else
    {
      info.emplace( "semver", query.getStr( 6 ) );
    }

  if ( query.isNull( 7 ) )
    {
      info.emplace( "license", nullptr );
    }
  else
    {
      info.emplace( "license", query.getStr( 7 ) );
    }

  if ( query.isNull( 10 ) )
    {
      info.emplace( "broken", nullptr );
    }
  else
    {
      info.emplace( "broken", ( query.getInt( 10 ) != 0 ) );
    }

  if ( query.isNull( 11 ) )
    {
      info.emplace( "unfree", nullptr );
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
DrvDb::getDrvInfo( std::string_view                 subtree
                 , std::string_view                 system
                 , const std::vector<std::string> & path
                 )
{
  nlohmann::json relPath = path;
  auto state( this->_state->lock() );
  nix::SQLiteStmt::Use query =
    state->queryDrvInfo.use()( subtree )( system )( relPath.dump() );
  if ( ! query.next() ) { return std::nullopt; }
  return infoFromQuery( query );
}


/* -------------------------------------------------------------------------- */

  std::list<nlohmann::json>
DrvDb::getDrvInfos( std::string_view subtree, std::string_view system )
{
  std::list<nlohmann::json> rsl;
  auto state( this->_state->lock() );
  auto query = state->queryDrvInfos.use()( subtree )( system );
  while ( query.next() ) { rsl.push_back( infoFromQuery( query ) ); }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  progress_status
DrvDb::getProgress( std::string_view subtree, std::string_view system )
{
  auto state( this->_state->lock() );
  auto query = state->queryProgress.use()( subtree )( system );
  if ( ! query.next() ) { return DBPS_NONE; }
  return (progress_status) query.getInt( 0 );
}


/* -------------------------------------------------------------------------- */

  progress_status
DrvDb::setProgress( std::string_view subtree
                  , std::string_view system
                  , progress_status  status
                  )
{
  auto state( this->_state->lock() );
  auto query = state->queryProgress.use()( subtree )( system );
  progress_status old = DBPS_NONE;
  if ( query.next() ) { old = (progress_status) query.getInt( 0 ); }
  doSQLite( [&](){
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
  auto state( this->_state->lock() );
  auto query = state->queryProgress.use()( subtree )( system );
  progress_status old = DBPS_NONE;
  if ( query.next() ) { old = (progress_status) query.getInt( 0 ); }
  if ( status <= old ) { return old; }
  doSQLite( [&](){
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

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
