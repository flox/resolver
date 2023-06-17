/* ========================================================================== *
 *
 * NOTE: Currently unused.
 * TODO: Integrate into `packagePredicate' and `DrvDb'.
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
#include "flox/types.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class Package {
  private:
    Cursor                     _cursor;
    std::vector<nix::Symbol>   _path;
    nix::SymbolTable         * _symtab;

    bool _hasMetaAttr    = false;
    bool _hasPnameAttr   = false;
    bool _hasVersionAttr = false;

    nix::DrvName               _dname;
    std::optional<std::string> _semver;
    std::string                _system;
    subtree_type               _subtree;

    void init( bool checkDrv = true );


  public:
    Package( Cursor             cursor
           , nix::SymbolTable * symtab
           , bool               checkDrv = true
           )
      : _cursor( cursor ), _path( cursor->getAttrPath() ), _symtab( symtab )
      , _dname( cursor->getAttr( "name" )->getString() )
    {
      this->init( checkDrv );
    }

    Package(       Cursor                     cursor
           , const std::vector<nix::Symbol> & path
           ,       nix::SymbolTable         * symtab
           ,       bool                       checkDrv = true
           )
      : _cursor( cursor ), _path( path ), _symtab( symtab )
      , _dname( cursor->getAttr( "name" )->getString() )
    {
      this->init( checkDrv );
    }

    /* Copy */
    Package( const Package & p )
      : _cursor( p._cursor )
      , _path( p._path )
      , _symtab( p._symtab )
      , _dname( p._dname.fullName )
      , _hasMetaAttr( p._hasMetaAttr )
      , _hasPnameAttr( p._hasPnameAttr )
      , _hasVersionAttr( p._hasVersionAttr )
      , _semver( p._semver )
      , _system( p._system )
      , _subtree( p._subtree )
    {}

    /* Move */
    Package( Package && p ) noexcept
      : _cursor( std::move( p._cursor ) )
      , _path( std::move( p._path ) )
      , _symtab( std::move( p._symtab ) )
      , _dname( p._dname.fullName )
      , _hasMetaAttr( std::move( p._hasMetaAttr ) )
      , _hasPnameAttr( std::move( p._hasPnameAttr ) )
      , _hasVersionAttr( std::move( p._hasVersionAttr ) )
      , _semver( std::move( p._semver ) )
      , _system( std::move( p._system ) )
      , _subtree( std::move( p._subtree ) )
    {}

    std::vector<nix::Symbol>   getPath()             const;
    Cursor                     getCursor()           const;
    subtree_type               getSubtreeType()      const;
    std::optional<std::string> getStability()        const;
    nix::DrvName               getParsedDrvName()    const;
    std::string                getFullName()         const;
    std::string                getPname()            const;
    std::string                getPkgAttrName()      const;
    std::optional<std::string> getVersion()          const;
    std::optional<std::string> getSemver()           const;
    std::optional<std::string> getLicense()          const;
    std::vector<std::string>   getOutputs()          const;
    std::vector<std::string>   getOutputsToInstall() const;
    std::optional<bool>        isBroken()            const;
    std::optional<bool>        isUnfree()            const;
    bool                       hasMetaAttr()         const;
    bool                       hasPnameAttr()        const;
    bool                       hasVersionAttr()      const;

    std::string toURIString( const FloxFlakeRef & ref ) const;
    //nlohmann::json toJSON()      const;
};


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
