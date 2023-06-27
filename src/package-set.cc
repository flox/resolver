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
    std::list<CachedPackage>   _pkgs;
    subtree_type               _subtree;
    std::string                _system;
    std::optional<std::string> _stability;
    FloxFlakeRef               _ref;

  public:
    std::string_view getType()    const override { return "raw";          }
    subtree_type     getSubtree() const override { return this->_subtree; }
    std::string_view getSystem()  const override { return this->_system;  }
    FloxFlakeRef     getRef()     const override { return this->_ref;     }

      std::optional<std::string_view>
    getStability() const override
    {
      if ( this->_stability.has_value() ) { return this->_stability; }
      else                                { return std::nullopt;     }
    }

    std::size_t size()  override { return this->_pkgs.size();  }
    std::size_t size()  const    { return this->_pkgs.size();  }
    bool        empty() override { return this->_pkgs.empty(); }
    bool        empty() const    { return this->_pkgs.empty(); }

      bool
    hasRelPath( const std::list<std::string_view> & path ) override
    {
      // TODO
      return false;
    }

      std::shared_ptr<Package>
    maybeGetRelPath( const std::list<std::string_view> & path ) override
    {
      // TODO
      return nullptr;
    }

      iterator
    begin() override
    {
      std::list<CachedPackage>::iterator it = this->_pkgs.begin();
      return iterator( [&]()
      {
        ++it;
        return std::shared_ptr<iterator::value_type>( & ( * it ) );
      } );
    }

      iterator
    end() override
    {
      std::list<CachedPackage>::iterator it = this->_pkgs.end();
      return iterator( [&]()
      {
        ++it;
        return std::shared_ptr<iterator::value_type>( & ( * it ) );
      } );
    }

      const_iterator
    begin() const override
    {
      std::list<CachedPackage>::const_iterator it = this->_pkgs.cbegin();
      return const_iterator( [&]()
      {
        ++it;
        return std::shared_ptr<const_iterator::value_type>( & ( * it ) );
      } );
    }

      const_iterator
    end() const override
    {
      std::list<CachedPackage>::const_iterator it = this->_pkgs.cend();
      return const_iterator( [&]()
      {
        ++it;
        return std::shared_ptr<const_iterator::value_type>( & ( * it ) );
      } );
    }

};  /* End class `RawPackageSet' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
