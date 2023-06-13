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
#include "flox/types.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class Package {
  private:
    FloxFlakeRef               _flake;
    Cursor                     _cursor;
    std::vector<nix::Symbol>   _path;
    nix::ref<nix::SymbolTable> _symtab;

    bool _hasMetaAttr    = false;
    bool _hasPnameAttr   = false;
    bool _hasVersionAttr = false;

    nix::DrvName               _dname;
    std::optional<std::string> _semver;
    std::string                _system;
    subtree_type               _subtree;

    void init();


  public:
    Package( const FloxFlakeRef               & flake
           ,       Cursor                       cursor
           ,       nix::ref<nix::SymbolTable>   symtab
           )
      : _flake( flake )
      , _cursor( cursor )
      , _path( cursor->getAttrPath() )
      , _symtab( symtab )
    {
      this->init();
    }

    Package( const FloxFlakeRef               & flake
           ,       Cursor                       cursor
           , const std::vector<nix::Symbol>   & path
           ,       nix::ref<nix::SymbolTable>   symtab
           )
      : _flake( flake ), _cursor( cursor ), _path( path ), _symtab( symtab )
    {
      this->init();
    }

    FloxFlakeRef               getFlakeRef()      const;
    std::vector<nix::Symbol>   getPath()          const;
    Cursor                     getCursor()        const;
    subtree_type               getSubtreeType()   const;
    nix::DrvName               getParsedDrvName() const;

    std::string                getFullName()         const;
    std::string                getPname()            const;
    std::optional<std::string> getVersion()          const;
    std::optional<std::string> getSemver()           const;
    std::optional<std::string> getLicense()          const;
    std::optional<bool>        isBroken()            const;
    std::optional<bool>        isUnfree()            const;
    std::vector<std::string>   getOutputs()          const;
    std::vector<std::string>   getOutputsToInstall() const;

    bool hasMetaAttr()    const;
    bool hasPnameAttr()   const;
    bool hasVersionAttr() const;

    std::string    toURIString() const;
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
