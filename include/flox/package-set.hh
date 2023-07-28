/* ========================================================================== *
 *
 * @file flox/package-set.hh
 *
 * @brief Abstract declaration of a package set.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <concepts>
#include <list>
#include <optional>
#include "flox/package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

class PackageSet {

  public:
    virtual std::string_view getType()    const = 0;
    virtual subtree_type     getSubtree() const = 0;
    virtual std::string_view getSystem()  const = 0;
    virtual FloxFlakeRef     getRef()     const = 0;
    virtual std::size_t      size()             = 0;

    virtual bool empty() { return this->size() <= 0; }

    virtual bool hasRelPath( const std::list<std::string_view> & path ) = 0;

    virtual std::optional<std::string_view> getStability() const = 0;

      virtual std::list<std::string_view>
    getPrefix() const
    {
      auto s = this->getStability();
      if ( s.has_value() )
        {
          return {
            subtreeTypeToString( this->getSubtree() )
          , this->getSystem()
          , s.value()
          };
        }
      else
        {
          return {
            subtreeTypeToString( this->getSubtree() )
          , this->getSystem()
          };
        }
    }

    virtual std::shared_ptr<Package> maybeGetRelPath(
      const std::list<std::string_view> & path
    ) = 0;

      virtual nix::ref<Package>
    getRelPath( const std::list<std::string_view> & path )
    {
      std::shared_ptr<Package> p = this->maybeGetRelPath( path );
      if ( p == nullptr )
        {
          std::string msg( "PackageSet::getRelPath(): No such path '" );
          msg += this->getRef().to_string();
          msg += "#";
          bool fst = true;
          for ( const auto & p : this->getPrefix() )
            {
              if ( fst ) { msg += p; fst = false; }
              else       { msg += p; msg += ".";  }
            }
          for ( const auto & p : path )
            {
              if ( p.find( '.' ) == p.npos )
                {
                  msg += '.';
                  msg += p;
                }
              else
                {
                  msg += ".\"";
                  msg += p;
                  msg += '"';
                }
            }
          msg += "'.";
          throw ResolverException( msg.c_str() );
        }
      return (nix::ref<Package>) p;
    }

};  /* End class `PackageSet' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
