/* ========================================================================== *
 *
 * NOTE: Currently unused.
 * TODO: Integrate into `packagePredicate' and `DrvDb'.
 *
 * -------------------------------------------------------------------------- */

#include "flox/package.hh"
#include "semver.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  void
Package::init( bool checkDrv )
{
  if ( this->_path.size() < 3 )
    {
      throw ResolverException(
        "Package::init(): Package attribute paths must have at least 3 "
        "elements - the path '" + this->_cursor->getAttrPathStr() +
        "' is too short."
      );
    }

  if ( checkDrv && ( ! this->_cursor->isDerivation() ) )
    {
      throw ResolverException(
        "Package::init(): Packages must be derivations but the attrset at '"
        + this->_cursor->getAttrPathStr() +
        "' does not set `.type = \"derivation\"'."
      );
    }

  /* Subtree type */
  if ( this->_pathS[0] == "packages" )
    {
      this->_subtree = ST_PACKAGES;
    }
  else if ( this->_pathS[0] == "catalog" )
    {
      this->_subtree = ST_CATALOG;
    }
  else if ( this->_pathS[0] == "legacyPackages" )
    {
      this->_subtree = ST_LEGACY;
    }
  else
    {
      throw ResolverException(
        "Package::init(): Invalid subtree name '" + this->_pathS[0] + "' at "
        "path '" + this->_cursor->getAttrPathStr() + "'."
      );
    }

  this->_system = this->_pathS[1];

  MaybeCursor c = this->_cursor->maybeGetAttr( "meta" );
  this->_hasMetaAttr = c != nullptr;

  c = this->_cursor->maybeGetAttr( "pname" );
  this->_hasPnameAttr = c != nullptr;

  /* Version and Semver */
  c = this->_cursor->maybeGetAttr( "version" );
  if ( c != nullptr )
    {
      this->_hasPnameAttr = true;
      std::string v = c->getString();
      if ( v.empty() )   { v = this->_dname.version; }
      if ( ! v.empty() ) { this->_semver = coerceSemver( v ); }
    }
  else if ( ! this->_dname.version.empty() )
    {
      this->_semver = coerceSemver( this->_dname.version );
    }
}


/* -------------------------------------------------------------------------- */

Cursor        Package::getCursor()      const { return this->_cursor; }
subtree_type  Package::getSubtreeType() const { return this->_subtree; }
std::string   Package::getFullName()    const { return this->_dname.fullName; }

std::vector<nix::Symbol>   Package::getPath()   const { return this->_path; }
std::optional<std::string> Package::getSemver() const { return this->_semver; }

bool Package::hasMetaAttr()    const { return this->_hasMetaAttr; }
bool Package::hasPnameAttr()   const { return this->_hasPnameAttr; }
bool Package::hasVersionAttr() const { return this->_hasVersionAttr; }


/* -------------------------------------------------------------------------- */

  std::vector<nix::SymbolStr>
Package::getPathStrs() const
{
  return this->_pathS;
}


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

  std::optional<std::string>
Package::getStability() const
{
  if ( this->_subtree != ST_CATALOG ) { return std::nullopt; }
  return this->_pathS[2];
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
Package::toURIString( const FloxFlakeRef & ref ) const
{
  std::string uri = ref.to_string() + "#";
  for ( size_t i = 0; i < this->_pathS.size(); ++i )
    {
      uri += "\"" + this->_pathS[i] + "\"";
      if ( ( i + 1 ) < this->_pathS.size() ) uri += ".";
    }
  return uri;
}


/* -------------------------------------------------------------------------- */

  std::string
Package::getPkgAttrName() const
{
  if ( this->getSubtreeType() == ST_CATALOG )
    {
      return this->_pathS[this->_pathS.size() - 2];
    }
  else
    {
      return this->_pathS.back();
    }
}


/* -------------------------------------------------------------------------- */

// nlohmann::json Package::toJSON() const {}


/* -------------------------------------------------------------------------- */

  nlohmann::json
Package::getInfo() const
{
  return { { this->_pathS[1], {
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

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
