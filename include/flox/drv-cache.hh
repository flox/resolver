/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/eval-cache.hh>
#include <nix/sqlite.hh>
#include "flox/package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class DrvDb {

  private:
    std::atomic_bool failed { false };

    struct State {
      nix::SQLite                     db;
      nix::SQLiteStmt                 insertDrv;
      nix::SQLiteStmt                 queryDrvs;
      nix::SQLiteStmt                 insertDrvInfo;
      nix::SQLiteStmt                 queryDrvInfos;
      nix::SQLiteStmt                 insertProgress;
      nix::SQLiteStmt                 queryProgress;
      nix::SQLiteStmt                 insertFingerprint;
      nix::SQLiteStmt                 queryFingerprint;
      nix::SQLiteStmt                 queryVersionInfo;
      std::unique_ptr<nix::SQLiteTxn> txn;
    };

    std::unique_ptr<nix::Sync<State>> _state;

  public:

    DrvDb( const nix::flake::Fingerprint & fingerprint );
    ~DrvDb();
    template<typename F> uint64_t doSQLite( F && fun );
    uint64_t setDrv( const Package & p );
    uint64_t setDrvInfo( const Package & p );

    std::optional<nlohmann::json> getDrvInfo(
      std::string_view                 subtree
    , std::string_view                 system
    , const std::vector<std::string> & path
    );

    // TODO: setProgress
    // TODO: getProgress
    // TODO: setFingerprint
    // TODO: getFingerprint

};  /* End class `DrvDb' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
