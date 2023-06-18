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
#include "semver.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class Package {
  public:
    virtual std::vector<std::string>    getPathStrs()         const = 0;
    virtual std::string                 getFullName()         const = 0;
    virtual std::string                 getPname()            const = 0;
    virtual std::optional<std::string>  getVersion()          const = 0;
    virtual std::optional<std::string>  getLicense()          const = 0;
    virtual std::vector<std::string>    getOutputs()          const = 0;
    virtual std::vector<std::string>    getOutputsToInstall() const = 0;
    virtual std::optional<bool>         isBroken()            const = 0;
    virtual std::optional<bool>         isUnfree()            const = 0;
    virtual bool                        hasMetaAttr()         const = 0;
    virtual bool                        hasPnameAttr()        const = 0;
    virtual bool                        hasVersionAttr()      const = 0;


      virtual subtree_type
    getSubtreeType() const
    {
      std::vector<std::string> pathS = this->getPathStrs();
      if ( pathS[0] == "legacyPackage" ) { return ST_LEGACY; }
      if ( pathS[0] == "package" )       { return ST_PACKAGES; }
      if ( pathS[0] == "catalog" )       { return ST_PACKAGES; }
      throw ResolverException(
        "Package::getSubtreeType(): Unrecognized subtree '" + pathS[0] + "'."
      );
    }

      virtual std::optional<std::string>
    getStability() const
    {
      if ( this->getSubtreeType() != ST_CATALOG ) { return std::nullopt; }
      return this->getPathStrs()[2];
    }

      virtual nix::DrvName
    getParsedDrvName() const
    {
      return nix::DrvName( this->getFullName() );
    }

      virtual std::string
    getPkgAttrName() const
    {
      std::vector<std::string> pathS = this->getPathStrs();
      if ( this->getSubtreeType() != ST_CATALOG )
        {
          return pathS[pathS.size() - 2];
        }
      return pathS[pathS.size() - 1];
    }

      virtual std::optional<std::string>
    getSemver() const
    {
      std::optional<std::string> version = this->getVersion();
      if ( ! version.has_value() ) { return std::nullopt; }
      return coerceSemver( version.value() );
    }

      virtual std::string
    toURIString( const FloxFlakeRef & ref ) const
    {
      std::string uri                = ref.to_string() + "#";
      std::vector<std::string> pathS = this->getPathStrs();
      for ( size_t i = 0; i < pathS.size(); ++i )
        {
          uri += "\"" + pathS[i] + "\"";
          if ( ( i + 1 ) < pathS.size() ) uri += ".";
        }
      return uri;
    }

      virtual nlohmann::json
    getInfo() const
    {
      return { { this->getPathStrs()[1], {
        { "name",    this->getFullName() }
      , { "pname",   this->getPname() }
      , { "version", this->getVersion().value_or( nullptr ) }
      , { "semver",  this->getSemver().value_or( nullptr ) }
      , { "outputs", this->getOutputs() }
      , { "license", this->getLicense().value_or( nullptr ) }
      , { "broken",  this->isBroken().value_or( false ) }
      , { "unfree",  this->isUnfree().value_or( false ) }
      } } };
    }


/* -------------------------------------------------------------------------- */

};  /* End class `Package' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
