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

CREATE TABLE IF NOT EXISTS Progress (
  subtree  TEXT     NOT NULL
, system   TEXT     NOT NULL
, status   INTEGER  DEFAULT 0
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

struct DrvDb {

  private:
    std::atomic_bool failed { false };

    struct State {
      nix::SQLite                     db;
      nix::SQLiteStmt                 insertDrv;
      nix::SQLiteStmt                 queryDrvs;
      nix::SQLiteStmt                 insertProgress;
      nix::SQLiteStmt                 queryProgress;
      nix::SQLiteStmt                 insertFingerprint;
      nix::SQLiteStmt                 queryFingerprint;
      nix::SQLiteStmt                 queryVersionInfo;
      std::unique_ptr<nix::SQLiteTxn> txn;
    };

    std::unique_ptr<nix::Sync<State>> _state;

  public:
    DrvDb( const nix::flake::Fingerprint & fingerprint )
      : _state( std::make_unique<nix::Sync<State>>() )
    {
      auto state( _state->lock() );

      nix::Path cacheDir = nix::getCacheDir() + "/flox/drv-cache-v0";
      nix::createDirs( cacheDir );

      nix::Path dbPath = cacheDir + "/" +
                         fingerprint.to_string( nix::Base16, false ) +
                         ".sqlite";

      state->db = nix::SQLite( dbPath );
      state->db.isCache();
      state->db.exec( schema );

      state->insertDrv.create(
        state->db
      , "INSERT OR REPLACE INTO Derivations ( subtree, system, path ) VALUES "
        "( ?, ?, ? )"
      );

      state->insertProgress.create(
        state->db
      , "INSERT OR REPLACE INTO Progress ( subtree, system, status ) VALUES "
        "( ?, ?, ? )"
      );

      state->insertFingerprint.create(
        state->db
      , "INSERT OR REPLACE INTO Fingerprint VALUES ( ? )"
      );

      state->queryDrvs.create(
        state->db
      , "SELECT * FROM Derivations WHERE ( subtree = ? ) AND ( system = ? )"
      );

      state->queryProgress.create(
        state->db
      , "SELECT * FROM Progress WHERE ( subtree = ? ) AND ( system = ? )"
      );

      state->queryFingerprint.create(
        state->db
      , "SELECT fingerprint Fingerprint LIMIT 1"
      );

      state->queryVersionInfo.create(
        state->db
      , "SELECT version FROM VersionInfo WHERE ( id = ? )"
      );

      state->txn = std::make_unique<nix::SQLiteTxn>( state->db );
    }

    ~DrvDb()
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

};


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
