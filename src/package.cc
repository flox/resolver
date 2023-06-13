/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  void
Package::init()
{
}


/* -------------------------------------------------------------------------- */

FloxFlakeRef  Package::getFlakeRef()    const { return this->_flake; }
Cursor        Package::getCursor()      const { return this->_cursor; }
subtree_type  Package::getSubtreeType() const { return this->_subtree; }
std::string   Package::getFullName()    const { return this->_dname.fullName; }

std::vector<nix::Symbol>   Package::getPath()   const { return this->_path; }
std::optional<std::string> Package::getSemver() const { return this->_semver; }

bool Package::hasMetaAttr()    const { return this->_hasMetaAttr; }
bool Package::hasPnameAttr()   const { return this->_hasPnameAttr; }
bool Package::hasVersionAttr() const { return this->_hasVersionAttr; }

/* -------------------------------------------------------------------------- */

  nix::DrvName
Package::getParsedDrvName() const
{
  return nix::DrvName( this->_dname.fullName );
}

  std::string
Package::getPname() const
{
  if ( this->_hasPnameAttr )
    {
      return this->_cursor->getAttr( "pname" )->getString();
    }
  else
    {
      return this->_dname.name;
    }
}


  std::optional<std::string>
Package::getVersion() const
{
  if ( this->_subtree == ST_CATALOG )
    {
      std::string last =
        (* this->_symtab )[this->_path[this->_path.size() - 1]];
      if ( ( last != "latest" ) && ( last != "unknown" ) )
        {
          for ( size_t i = 0; i < last.size(); ++i )
            {
              if ( last[i] == '_' ) { last[i] = '.'; }
            }
          return last;
        }
    }
  if ( this->_hasVersionAttr )
    {
      return this->_cursor->getAttr( "version" )->getString();
    }
  else if ( this->_dname.version.empty() )
    {
      return std::nullopt;
    }
  else
    {
      return this->_dname.version;
    }
}


/* -------------------------------------------------------------------------- */

  std::vector<std::string>
Package::getOutputs() const
{
  MaybeCursor o = this->_cursor->maybeGetAttr( "outputs" );
  if ( o == nullptr )
    {
      return { "out" };
    }
  else
    {
      return o->getListOfStrings();
    }
}


  std::vector<std::string>
Package::getOutputsToInstall() const
{
  if ( this->_hasMetaAttr )
    {
      MaybeCursor m =
        this->_cursor->getAttr( "meta" )->maybeGetAttr( "outputsToInstall" );
      if ( m != nullptr )
        {
          return m->getListOfStrings();
        }
    }
  std::vector<std::string> rsl;
  for ( std::string o : this->getOutputs() )
    {
      rsl.push_back( o );
      if ( o == "out" ) { break; }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::optional<std::string>
Package::getLicense() const
{
  if ( ! this->_hasMetaAttr ) { return std::nullopt; }
  MaybeCursor l = this->_cursor->getAttr( "meta" )->maybeGetAttr( "license" );
  if ( l == nullptr ) { return std::nullopt; }
  return l->getAttr( "spdxId" )->getString();
}


/* -------------------------------------------------------------------------- */

  std::optional<bool>
Package::isBroken() const
{
  if ( ! this->_hasMetaAttr ) { return std::nullopt; }
  MaybeCursor b = this->_cursor->getAttr( "meta" )->maybeGetAttr( "broken" );
  if ( b == nullptr ) { return std::nullopt; }
  return b->getBool();
}

  std::optional<bool>
Package::isUnfree() const
{
  if ( ! this->_hasMetaAttr ) { return std::nullopt; }
  MaybeCursor u = this->_cursor->getAttr( "meta" )->maybeGetAttr( "unfree" );
  if ( u == nullptr ) { return std::nullopt; }
  return u->getBool();
}


/* -------------------------------------------------------------------------- */

  std::string
Package::toURIString() const
{
  std::string uri = this->_flake.to_string() + "#";
  for ( size_t i = 0; i < this->_path.size(); ++i )
    {
      uri += "\"" + ( * this->_symtab )[this->_path[i]] + "\"";
      if ( ( i + 1 ) < this->_path.size() ) uri += ".";
    }
  return uri;
}


/* -------------------------------------------------------------------------- */

// nlohmann::json Package::toJSON() const {}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
