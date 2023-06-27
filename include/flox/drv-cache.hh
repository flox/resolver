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

#define FLOX_DRVDB_SCHEMA_VERSION  "0.1.0"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class DrvDb;
class RawPackageSet;

/* -------------------------------------------------------------------------- */

class CachedPackage : public Package {
  protected:
    std::vector<std::string>    _pathS;

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
    bool                        _hasMetaAttr;
    bool                        _hasPnameAttr;
    bool                        _hasVersionAttr;

  public:
      std::vector<std::string>
    getPathStrs() const override
    {
      return this->_pathS;
    }
      std::string
    getFullName() const override
    {
      return this->_fullname;
    }
      std::string
    getPname() const override
    {
      return this->_pname;
    }
      std::optional<std::string>
    getVersion() const override
    {
      return this->_version;
    }
      std::optional<std::string>
    getSemver() const override
    {
      return this->_semver;
    }
      std::optional<std::string>
    getLicense() const override
    {
      return this->_license;
    }
      std::vector<std::string>
    getOutputs() const override
    {
      return this->_outputs;
    }
      std::optional<bool>
    isBroken() const override
    {
      return this->_broken;
    }
      std::optional<bool>
    isUnfree() const override
    {
      return this->_unfree;
    }
      bool
    hasMetaAttr() const override
    {
      return this->_hasMetaAttr;
    }
      bool
    hasPnameAttr() const override
    {
      return this->_hasPnameAttr;
    }
      bool
    hasVersionAttr() const override
    {
      return this->_hasVersionAttr;
    }
      std::vector<std::string>
    getOutputsToInstall() const override
    {
      return this->_outputsToInstall;
    }

    CachedPackage(       DrvDb                    & db
                 ,       std::string_view           subtree
                 ,       std::string_view           system
                 , const std::vector<std::string> & path
                 );

    CachedPackage( const nlohmann::json & drvInfo );

    friend RawPackageSet;

};  /* End class `CachedPackage' */


/* -------------------------------------------------------------------------- */

class DrvDb {

  private:
    std::atomic_bool failed { false };

  public:
    struct State {
      nix::SQLite                     db;
      nix::SQLiteStmt                 insertDrv;
      nix::SQLiteStmt                 hasDrv;
      nix::SQLiteStmt                 queryDrvs;
      nix::SQLiteStmt                 insertDrvInfo;
      nix::SQLiteStmt                 queryDrvInfo;
      nix::SQLiteStmt                 queryDrvInfos;
      nix::SQLiteStmt                 insertProgress;
      nix::SQLiteStmt                 queryProgress;
      nix::SQLiteStmt                 queryProgresses;
      nix::SQLiteStmt                 insertFingerprint;
      nix::SQLiteStmt                 queryFingerprint;
      nix::SQLiteStmt                 queryVersionInfo;
      std::unique_ptr<nix::SQLiteTxn> txn;
    };

  private:
    std::unique_ptr<nix::Sync<State>> _state;

  public:

    DrvDb( const nix::flake::Fingerprint & fingerprint );
    ~DrvDb();

    void startCommit();
    void endCommit();

    nix::Sync<DrvDb::State>::Lock getDbState();

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

};  /* End class `DrvDb' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
