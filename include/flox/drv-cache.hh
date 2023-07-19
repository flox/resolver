/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/eval-cache.hh>
#include "flox/sqlite.hh"
#include "flox/package.hh"
#include "flox/raw-package.hh"


/* -------------------------------------------------------------------------- */

#define FLOX_DRVDB_SCHEMA_VERSION  "0.1.0"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class DrvDb;
class RawPackageSet;

/* -------------------------------------------------------------------------- */

class CachedPackage : public RawPackage {
  public:
    CachedPackage() : RawPackage() {}
    CachedPackage( const nlohmann::json &  drvInfo ) : RawPackage( drvInfo ) {};
    CachedPackage(       nlohmann::json && drvInfo ) : RawPackage( drvInfo ) {};
    CachedPackage(       DrvDb                    & db
                 ,       std::string_view           subtree
                 ,       std::string_view           system
                 , const std::vector<std::string> & path
                 );
};


/* -------------------------------------------------------------------------- */

std::string getDrvDbName( const nix::flake::Fingerprint & fingerprint );

  static inline std::string
getDrvDbName( const nix::flake::LockedFlake & flake )
{
  return getDrvDbName( flake.getFingerprint() );
}


/* -------------------------------------------------------------------------- */

class DrvDb {

  public:
    struct State {
      sqlite::SQLiteDb                db;
      std::unique_ptr<nix::SQLiteTxn> txn;

      /* Inserts */
      nix::SQLiteStmt insertDrvInfo;
      nix::SQLiteStmt insertDrv;
      nix::SQLiteStmt insertProgress;

      /* Queries */
      nix::SQLiteStmt queryVersionInfo;

      nix::SQLiteStmt hasDrv;
      nix::SQLiteStmt queryDrvs;
      nix::SQLiteStmt countDrvs;
      nix::SQLiteStmt countDrvsStability;

      nix::SQLiteStmt queryDrvInfo;
      nix::SQLiteStmt queryDrvInfos;
      nix::SQLiteStmt queryDrvInfosStability;
      nix::SQLiteStmt countDrvInfos;
      nix::SQLiteStmt countDrvInfosStability;

      nix::SQLiteStmt queryProgress;
      nix::SQLiteStmt queryProgresses;

    };

  private:
    std::atomic_bool                  failed { false };
    std::unique_ptr<nix::Sync<State>> _state;
    bool                              _write;

  public:
    const nix::flake::Fingerprint fingerprint;

    DrvDb( const nix::flake::Fingerprint & fingerprint
         ,       bool                      create      = true
         ,       bool                      write       = true
         ,       bool                      trace       = false
         );

    ~DrvDb()
    {
      try { this->endCommit(); } catch( ... ) {}
    }

    void startCommit();
    void endCommit();

      nix::Sync<DrvDb::State>::Lock
    getDbState()
    {
      return this->_state->lock();
    }

    bool isWritable() const { return this->_write; }

    template<typename F> uint64_t doSQLite( F && fun );
    void setDrv( const Package & p );
    void setDrvInfo( const Package & p );

    void setDrv(       std::string_view           subtree
               ,       std::string_view           system
               , const std::vector<std::string> & path
               );

    std::optional<bool> hasDrv(       std::string_view           subtree
                              ,       std::string_view           system
                              , const std::vector<std::string> & path
                              );

    std::optional<std::list<std::vector<std::string>>> getDrvPaths(
      std::string_view subtree
    , std::string_view system
    );

    std::optional<nlohmann::json> getDrvInfo(
            std::string_view           subtree
    ,       std::string_view           system
    , const std::vector<std::string> & path
    );

    std::list<nlohmann::json> getDrvInfos(
      std::string_view subtree
    , std::string_view system
    );

    nix::SQLiteStmt::Use useDrvInfos(
      std::string_view subtree
    , std::string_view system
    );

    std::list<nlohmann::json> getDrvInfosStability(
      std::string_view system
    , std::string_view stability
    );

    nix::SQLiteStmt::Use useDrvInfosStability(
      std::string_view system
    , std::string_view stability
    );

    /* Get status of a subtree/system collection. */
    progress_status getProgress( std::string_view subtree
                               , std::string_view system
                               );
    /* Set status of a subtree/system collection, returning the old value. */
    progress_status setProgress( std::string_view subtree
                               , std::string_view system
                               , progress_status  status
                               );
    /* Set status of a subtree/system collection if `status' is "higher" than
     * the existing value.
     * Returns the old value. */
    progress_status promoteProgress( std::string_view subtree
                                   , std::string_view system
                                   , progress_status  status
                                   );
  std::unordered_map<std::string
                    , std::unordered_map<std::string
                                        , progress_status
                                        >
                    >                                     getProgresses();

  std::size_t countDrvs( std::string_view subtree
                       , std::string_view system
                       );
  std::size_t countDrvsStability( std::string_view system
                                , std::string_view stability
                                );
  std::size_t countDrvInfos( std::string_view subtree
                           , std::string_view system
                           );
  std::size_t countDrvInfosStability( std::string_view system
                                    , std::string_view stability
                                    );

};  /* End class `DrvDb' */


/* -------------------------------------------------------------------------- */

nlohmann::json infoFromQuery( nix::SQLiteStmt::Use & query );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
