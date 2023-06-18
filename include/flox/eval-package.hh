/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <nix/eval-cache.hh>
#include <nix/names.hh>
#include <unordered_map>
#include <unordered_set>
#include "flox/package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class EvalPackage : public Package {
  private:
    Cursor                   _cursor;
    std::vector<nix::Symbol> _path;
    std::vector<std::string> _pathS;

    bool _hasMetaAttr    = false;
    bool _hasPnameAttr   = false;
    bool _hasVersionAttr = false;

    nix::DrvName               _dname;
    std::optional<std::string> _semver;
    std::string                _system;
    subtree_type               _subtree;

    void init( bool checkDrv = true );


  public:
    EvalPackage( Cursor             cursor
               , nix::SymbolTable * symtab
               , bool               checkDrv = true
               )
      : _cursor( cursor )
      , _path( cursor->getAttrPath() )
      , _dname( cursor->getAttr( "name" )->getString() )
    {
      for ( auto & p : symtab->resolve( cursor->getAttrPath() ) )
        {
          this->_pathS.push_back( p );
        }
      this->init( checkDrv );
    }

    EvalPackage(       Cursor                     cursor
               , const std::vector<nix::Symbol> & path
               ,       nix::SymbolTable         * symtab
               ,       bool                       checkDrv = true
               )
      : _cursor( cursor )
      , _path( path )
      , _dname( cursor->getAttr( "name" )->getString() )
    {
      for ( auto & p : symtab->resolve( cursor->getAttrPath() ) )
        {
          this->_pathS.push_back( p );
        }
      this->init( checkDrv );
    }

    /* Copy */
    EvalPackage( const EvalPackage & p ) noexcept
      : _cursor( p._cursor )
      , _path( p._path )
      , _pathS( p._pathS )
      , _hasMetaAttr( p._hasMetaAttr )
      , _hasPnameAttr( p._hasPnameAttr )
      , _hasVersionAttr( p._hasVersionAttr )
      , _semver( p._semver )
      , _system( p._system )
      , _subtree( p._subtree )
    {
      this->_dname.fullName = p._dname.fullName;
      this->_dname.name     = p._dname.name;
      this->_dname.version  = p._dname.version;
    }

    /* Move */
    EvalPackage( EvalPackage && p ) noexcept
      : _cursor( std::move( p._cursor ) )
      , _path( std::move( p._path ) )
      , _pathS( std::move( p._pathS ) )
      , _hasMetaAttr( std::move( p._hasMetaAttr ) )
      , _hasPnameAttr( std::move( p._hasPnameAttr ) )
      , _hasVersionAttr( std::move( p._hasVersionAttr ) )
      , _semver( std::move( p._semver ) )
      , _system( std::move( p._system ) )
      , _subtree( std::move( p._subtree ) )
    {
      this->_dname.fullName = p._dname.fullName;
      this->_dname.name     = p._dname.name;
      this->_dname.version  = p._dname.version;
    }

      EvalPackage &
    operator=( const EvalPackage & p ) noexcept
    {
      this->_cursor         = p._cursor;
      this->_path           = p._path;
      this->_pathS          = p._pathS;
      this->_dname.fullName = p._dname.fullName;
      this->_dname.name     = p._dname.name;
      this->_dname.version  = p._dname.version;
      this->_dname.hits     = p._dname.hits;
      this->_hasMetaAttr    = p._hasMetaAttr;
      this->_hasPnameAttr   = p._hasPnameAttr;
      this->_hasVersionAttr = p._hasVersionAttr;
      this->_semver         = p._semver;
      this->_system         = p._system;
      this->_subtree        = p._subtree;
      return * this;
    }

      EvalPackage &
    operator=( EvalPackage && p ) noexcept
    {
      this->_cursor         = std::move( p._cursor );
      this->_path           = std::move( p._path );
      this->_pathS          = std::move( p._pathS );
      this->_dname.fullName = std::move( p._dname.fullName );
      this->_dname.name     = std::move( p._dname.name );
      this->_dname.version  = std::move( p._dname.version );
      this->_dname.hits     = std::move( p._dname.hits );
      this->_hasMetaAttr    = std::move( p._hasMetaAttr );
      this->_hasPnameAttr   = std::move( p._hasPnameAttr );
      this->_hasVersionAttr = std::move( p._hasVersionAttr );
      this->_semver         = std::move( p._semver );
      this->_system         = std::move( p._system );
      this->_subtree        = std::move( p._subtree );
      return * this;
    }

    std::vector<nix::Symbol>    getPath()             const;
    std::vector<std::string>    getPathStrs()         const;
    Cursor                      getCursor()           const;
    subtree_type                getSubtreeType()      const;
    std::optional<std::string>  getStability()        const;
    nix::DrvName                getParsedDrvName()    const;
    std::string                 getFullName()         const;
    std::string                 getPname()            const;
    std::optional<std::string>  getVersion()          const;
    std::optional<std::string>  getSemver()           const;
    std::optional<std::string>  getLicense()          const;
    std::vector<std::string>    getOutputs()          const;
    std::vector<std::string>    getOutputsToInstall() const;
    std::optional<bool>         isBroken()            const;
    std::optional<bool>         isUnfree()            const;
    bool                        hasMetaAttr()         const;
    bool                        hasPnameAttr()        const;
    bool                        hasVersionAttr()      const;
};


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
