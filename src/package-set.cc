/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/package-set.hh"
#include "flox/drv-cache.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  std::string
FlakeRefWithPath::toString() const
{
  std::string rsl = this->ref.to_string();
  if ( this->path.empty() ) { return rsl; }
  rsl += "#";
  bool first = true;
  for ( const auto & p : this->path )
    {
      if ( first ) { rsl += "\"" + p + "\""; first = false; }
      else         { rsl += ".\"" + p + "\""; }
    }
  return rsl;
}

  std::string
to_string( const FlakeRefWithPath & rp )
{
  return rp.toString();
}


/* -------------------------------------------------------------------------- */

class RawPackageSet : public PackageSet {

  protected:
    std::list<CachedPackage> _pkgs;
    std::string              _subtree;
    std::string              _system;
    FloxFlakeRef             _ref;

  public:
    std::string_view getType()    const override { return "raw";          }
    std::string_view getSubtree() const override { return this->_subtree; }
    std::string_view getSystem()  const override { return this->_system;  }
    FloxFlakeRef     getRef()     const override { return this->_ref;     }

    std::size_t size()  override { return this->_pkgs.size();  }
    std::size_t size()  const    { return this->_pkgs.size();  }
    bool        empty() override { return this->_pkgs.empty(); }
    bool        empty() const    { return this->_pkgs.empty(); }

      bool
    hasRelPath( std::list<std::string_view> path ) override
    {
      // TODO
      return false;
    }

      std::optional<Package *>
    maybeGetRelPath( std::list<std::string_view> path ) override
    {
      // TODO
      return std::nullopt;
    }

      FlakeRefWithPath
    getRefWithPath() const override
    {
      return FlakeRefWithPath {
        this->getRef()
      , std::list { this->_subtree, this->_system }
      };
    }


      PkgIter
    begin() override
    {
      std::list<CachedPackage>::iterator it = this->_pkgs.begin();
      return PkgIter( [&]() { ++it; return & ( * it ); } );
    }

      PkgIter
    end() override
    {
      std::list<CachedPackage>::iterator it = this->_pkgs.end();
      return PkgIter( [&]() { ++it; return & ( * it ); } );
    }

};


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
