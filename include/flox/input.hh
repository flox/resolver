/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <list>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <nix/eval-cache.hh>
#include "flox/types.hh"
#include "flox/exceptions.hh"
#include "flox/package-set.hh"
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

class Input {

  protected:

    std::shared_ptr<nix::flake::LockedFlake> _lockedFlake;


/* -------------------------------------------------------------------------- */

  public:

    Input( std::string_view refURI );
    Input( const resolve::FloxFlakeRef & ref );


/* -------------------------------------------------------------------------- */

      nix::Hash
    getFingerprint() const
    {
      return this->_lockedFlake->getFingerprint();
    }


/* -------------------------------------------------------------------------- */

    virtual std::unordered_set<resolve::subtree_type> getSubtrees() = 0;

      virtual bool
    hasSubtree( const resolve::subtree_type & subtree )
    {
      const auto sts = this->getSubtrees();
      return sts.find( subtree ) != sts.cend();
    }

      virtual bool
    hasSubtree( std::string_view subtree )
    {
      return this->hasSubtree( resolve::parseSubtreeType( subtree ) );
    }


/* -------------------------------------------------------------------------- */

    virtual std::unordered_set<std::string_view> getSystems(
      const resolve::subtree_type & subtree
    ) = 0;

      virtual std::unordered_set<std::string_view>
    getSystems( std::string_view subtree )
    {
      return this->getSystems( resolve::parseSubtreeType( subtree ) );
    }

      virtual bool
    hasSystem( const resolve::subtree_type & subtree, std::string_view system )
    {
      if ( ! this->hasSubtree( subtree ) ) { return false; }
      const auto ss = this->getSystems( subtree );
      return ss.find( system ) != ss.cend();
    }

      virtual bool
    hasSystem( std::string_view subtree, std::string_view system )
    {
      return this->hasSystem( resolve::parseSubtreeType( subtree ), system );
    }


/* -------------------------------------------------------------------------- */

      virtual std::unordered_set<std::string_view>
    getStabilities( std::string_view system )
    {
      return {};
    }

      virtual bool
    hasStability( std::string_view system, std::string_view stability )
    {
      if ( ! this->hasSystem( resolve::ST_CATALOG, system ) ) { return false; }
      const auto sts = this->getStabilities( system );
      return sts.find( stability ) != sts.cend();
    }


/* -------------------------------------------------------------------------- */

    virtual std::list<nix::ref<resolve::PackageSet>> getPackageSets() = 0;
    virtual std::shared_ptr<resolve::PackageSet>     getPackageSet(
      const resolve::subtree_type & subtree
    ,       std::string_view        system
    ) = 0;

      virtual std::shared_ptr<resolve::PackageSet>
    getPackageSet( std::string_view subtreeOrSystem
                 , std::string_view systemOrCatalog
                 )
    {
      if ( resolve::isPkgsSubtree( subtreeOrSystem ) )
        {
          return this->getPackageSet(
            resolve::parseSubtreeType( subtreeOrSystem )
          , systemOrCatalog
          );
        }
      else if ( this->hasStability( subtreeOrSystem, systemOrCatalog ) )
        {
          throw resolve::ResolverException(
            "Input::getPackageSet(): Cannot get catalog package sets with "
            "virtual implementation."
          );
        }
      else
        {
          return nullptr;
        }
    }


/* -------------------------------------------------------------------------- */

};  /* End class `Input' */


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
