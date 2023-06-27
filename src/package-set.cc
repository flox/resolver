/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/package-set.hh"
#include "flox/drv-cache.hh"


/* -------------------------------------------------------------------------- */

template<>
struct std::hash<std::list<std::string_view>>
{
    std::size_t
  operator()( const std::list<std::string_view> & lst ) const noexcept
  {
    if ( lst.empty() ) { return 0; }
    auto it = lst.begin();
    std::size_t h1 = std::hash<std::string_view>{}( * it );
    for ( ; it != lst.cend(); ++it )
      {
        h1 = ( h1 >> 1 ) ^ ( std::hash<std::string_view>{}( *it ) << 1 );
      }
    return h1;
  }
};


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
    std::unordered_map<std::list<std::string_view>, CachedPackage> _pkgs;
    subtree_type                                                   _subtree;
    std::string                                                    _system;
    std::optional<std::string>                                     _stability;
    FloxFlakeRef                                                   _ref;

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
      return this->_pkgs.find( path ) != this->_pkgs.cend();
    }

      std::shared_ptr<Package>
    maybeGetRelPath( const std::list<std::string_view> & path ) override
    {
      auto search = this->_pkgs.find( path );
      if ( search == this->_pkgs.cend() )
        {
          return nullptr;
        }
      else
        {
          return std::shared_ptr<Package>( (Package *) & search->second );
        }
    }

      nix::ref<Package>
    getRelPath( const std::list<std::string_view> & path ) override
    {
      return nix::ref<Package>( & this->_pkgs.at( path ) );
    }

      iterator
    begin() override
    {
      std::unordered_map<std::list<std::string_view>
                        , CachedPackage
                        >::iterator                  it = this->_pkgs.begin();
      return iterator( [&]()
      {
        ++it;
        return std::shared_ptr<iterator::value_type>( & it->second );
      } );
    }

      iterator
    end() override
    {
      std::unordered_map<std::list<std::string_view>
                        , CachedPackage
                        >::iterator                  it = this->_pkgs.end();
      return iterator( [&]()
      {
        ++it;
        return std::shared_ptr<iterator::value_type>( & it->second );
      } );
    }

      const_iterator
    begin() const override
    {
      std::unordered_map<std::list<std::string_view>
                        , CachedPackage
                        >::const_iterator            it = this->_pkgs.cbegin();
      return const_iterator( [&]()
      {
        ++it;
        return std::shared_ptr<const_iterator::value_type>( & it->second );
      } );
    }

      const_iterator
    end() const override
    {
      std::unordered_map<std::list<std::string_view>
                        , CachedPackage
                        >::const_iterator            it = this->_pkgs.cend();
      return const_iterator( [&]()
      {
        ++it;
        return std::shared_ptr<const_iterator::value_type>( & it->second );
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
