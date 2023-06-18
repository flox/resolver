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

class CachedPackage : public Package {
  private:
    std::vector<std::string>    _pathS;
    subtree_type                _subtree;
    std::string                 _fullname;
    std::string                 _pname;
    std::optional<std::string>  _version;
    std::optional<std::string>  _semver;
    std::optional<std::string>  _license;
    std::vector<std::string>    _outputs;
    std::vector<std::string>    _outputsToInstall;
    std::optional<bool>         _broken;
    std::optional<bool>         _unfree;
    bool                        _hasMeta;
    bool                        _hasPnameAttr;
    bool                        _hasVersionAttr;

  public:
    std::vector<std::string>    getPathStrs() const { return this->_pathS;    }
    std::string                 getFullName() const { return this->_fullname; }
    std::string                 getPname()    const { return this->_pname;    }
    std::optional<std::string>  getVersion()  const { return this->_version;  }
    std::optional<std::string>  getSemver()   const { return this->_semver;   }
    std::optional<std::string>  getLicense()  const { return this->_license;  }
    std::vector<std::string>    getOutputs()  const { return this->_outputs;  }
    std::optional<bool>         isBroken()    const { return this->_broken;   }
    std::optional<bool>         isUnfree()    const { return this->_unfree;   }

    bool hasMetaAttr()    const { return this->_hasMeta;        }
    bool hasPnameAttr()   const { return this->_hasPnameAttr;   }
    bool hasVersionAttr() const { return this->_hasVersionAttr; }

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

    CachedPackage( const nlohmann::json & info );

    CachedPackage( const CachedPackage & p ) noexcept
      : _pathS( p._pathS )
      , _subtree( p._subtree )
      , _fullname( p._fullname )
      , _pname( p._pname )
      , _version( p._version )
      , _semver( p._semver )
      , _license( p._license )
      , _outputs( p._outputs )
      , _outputsToInstall( p._outputsToInstall )
      , _broken( p._broken )
      , _unfree( p._unfree )
      , _hasMeta( p._hasMeta )
      , _hasPnameAttr( p._hasPnameAttr )
      , _hasVersionAttr( p._hasVersionAttr )
    {}

    CachedPackage( const CachedPackage && p ) noexcept
      : _pathS( std::move(  p._pathS ) )
      , _subtree( std::move(  p._subtree ) )
      , _fullname( std::move(  p._fullname ) )
      , _pname( std::move(  p._pname ) )
      , _version( std::move(  p._version ) )
      , _semver( std::move(  p._semver ) )
      , _license( std::move(  p._license ) )
      , _outputs( std::move(  p._outputs ) )
      , _outputsToInstall( std::move(  p._outputsToInstall ) )
      , _broken( std::move(  p._broken ) )
      , _unfree( std::move(  p._unfree ) )
      , _hasMeta( std::move(  p._hasMeta ) )
      , _hasPnameAttr( std::move(  p._hasPnameAttr ) )
      , _hasVersionAttr( std::move(  p._hasVersionAttr ) )
    {}

      CachedPackage &
    operator=( const CachedPackage & p ) noexcept
    {
      _pathS            = p._pathS;
      _subtree          = p._subtree;
      _fullname         = p._fullname;
      _pname            = p._pname;
      _version          = p._version;
      _semver           = p._semver;
      _license          = p._license;
      _outputs          = p._outputs;
      _outputsToInstall = p._outputsToInstall;
      _broken           = p._broken;
      _unfree           = p._unfree;
      _hasMeta          = p._hasMeta;
      _hasPnameAttr     = p._hasPnameAttr;
      _hasVersionAttr   = p._hasVersionAttr;
      return * this;
    }

      CachedPackage &
    operator=( CachedPackage && p ) noexcept
    {
      _pathS            = std::move( p._pathS );
      _subtree          = std::move( p._subtree );
      _fullname         = std::move( p._fullname );
      _pname            = std::move( p._pname );
      _version          = std::move( p._version );
      _semver           = std::move( p._semver );
      _license          = std::move( p._license );
      _outputs          = std::move( p._outputs );
      _outputsToInstall = std::move( p._outputsToInstall );
      _broken           = std::move( p._broken );
      _unfree           = std::move( p._unfree );
      _hasMeta          = std::move( p._hasMeta );
      _hasPnameAttr     = std::move( p._hasPnameAttr );
      _hasVersionAttr   = std::move( p._hasVersionAttr );
      return * this;
    }

};  /* End class `CachedPackage' */


/* -------------------------------------------------------------------------- */

class DrvDb {

  private:
    std::atomic_bool failed { false };

    struct State {
      nix::SQLite                     db;
      nix::SQLiteStmt                 insertDrv;
      nix::SQLiteStmt                 queryDrvs;
      nix::SQLiteStmt                 insertDrvInfo;
      nix::SQLiteStmt                 queryDrvInfo;
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

    std::list<nlohmann::json> getDrvInfos(
      std::string_view subtree
    , std::string_view system
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
