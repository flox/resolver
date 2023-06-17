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

class DrvDb;

/* -------------------------------------------------------------------------- */

// TODO: Define base class shared with existing `Package' class.
class CachedPackage {
  private:
    std::string                 _fullname;
    std::string                 _pname;
    std::optional<std::string>  _version;
    std::optional<std::string>  _semver;
    std::optional<std::string>  _license;
    std::vector<std::string>    _outputs;
    std::vector<std::string>    _outputsToInstall;
    std::optional<bool>         _broken;
    std::optional<bool>         _unfree;

  public:
    std::string                 getFullName() const { return this->_fullname; }
    std::string                 getPname()    const { return this->_pname;    }
    std::optional<std::string>  getVersion()  const { return this->_version;  }
    std::optional<std::string>  getSemver()   const { return this->_semver;   }
    std::optional<std::string>  getLicense()  const { return this->_license;  }
    std::vector<std::string>    getOutputs()  const { return this->_outputs;  }
    std::optional<bool>         isBroken()    const { return this->_broken;   }
    std::optional<bool>         isUnfree()    const { return this->_unfree;   }

      std::vector<std::string>
    getOutputsToInstall() const
    {
      return this->_outputsToInstall;
    }

    CachedPackage(       DrvDb                    & db
                 ,       std::string_view           subtree
                 ,       std::string_view           system
                 , const std::vector<std::string> & path
                 );
};


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

    typedef enum {
      DBPS_NONE       = 0
    , DBPS_PARTIAL    = 1
    , DBPS_PATHS_DONE = 2
    , DBPS_INFO_DONE  = 3
    }  progress_status;

    DrvDb( const nix::flake::Fingerprint & fingerprint );
    ~DrvDb();
    template<typename F> uint64_t doSQLite( F && fun );
    uint64_t setDrv( const Package & p );
    uint64_t setDrvInfo( const Package & p );

    std::optional<nlohmann::json> getDrvInfo(
            std::string_view           subtree
    ,       std::string_view           system
    , const std::vector<std::string> & path
    );

    /* Get status of a subtree/system collection. */
    progress_status getProgress( std::string_view subtree
                               , std::string_view system
                               );
    /* Set status of a subtree/system collection, returning the old value. */
    progress_status setProgress( std::string_view       subtree
                               , std::string_view       system
                               , DrvDb::progress_status status
                               );

};  /* End class `DrvDb' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
