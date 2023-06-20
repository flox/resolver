/* ========================================================================== *
 *
 * NOTE: Currently unused.
 * TODO: Integrate into `packagePredicate' and `DrvDb'.
 *
 * -------------------------------------------------------------------------- */

#include "flox/eval-package.hh"
#include "semver.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  void
EvalPackage::init( bool checkDrv )
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
        "EvalPackage::init(): Invalid subtree name '" + this->_pathS[0] +
        "' at path '" + this->_cursor->getAttrPathStr() + "'."
      );
    }

  this->_system = this->_pathS[1];

  // TODO: typecheck these attrs

  MaybeCursor c = this->_cursor->maybeGetAttr( "meta" );
  this->_hasMetaAttr = c != nullptr;

  c = this->_cursor->maybeGetAttr( "pname" );
  this->_hasPnameAttr = c != nullptr;

  /* Version and Semver */
  c = this->_cursor->maybeGetAttr( "version" );
  if ( c != nullptr )
    {
      std::string v;
      try
        {
          v = c->getString();
          this->_hasVersionAttr = true;
        }
      catch( ... ) {}
      if ( v.empty() ) { v = this->_dname.version; }
      else             { this->_semver = coerceSemver( v ); }
    }
  else if ( ! this->_dname.version.empty() )
    {
      this->_semver = coerceSemver( this->_dname.version );
    }
}


/* -------------------------------------------------------------------------- */

Cursor       EvalPackage::getCursor()      const { return this->_cursor; }
subtree_type EvalPackage::getSubtreeType() const { return this->_subtree; }

std::string EvalPackage::getFullName() const { return this->_dname.fullName; }

std::vector<nix::Symbol> EvalPackage::getPath() const { return this->_path; }

  std::optional<std::string>
EvalPackage::getSemver() const
{
  return this->_semver;
}

bool EvalPackage::hasMetaAttr()    const { return this->_hasMetaAttr; }
bool EvalPackage::hasPnameAttr()   const { return this->_hasPnameAttr; }
bool EvalPackage::hasVersionAttr() const { return this->_hasVersionAttr; }


/* -------------------------------------------------------------------------- */

  std::vector<std::string>
EvalPackage::getPathStrs() const
{
  return this->_pathS;
}


/* -------------------------------------------------------------------------- */

  nix::DrvName
EvalPackage::getParsedDrvName() const
{
  return nix::DrvName( this->_dname.fullName );
}

  std::string
EvalPackage::getPname() const
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
EvalPackage::getVersion() const
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
EvalPackage::getStability() const
{
  if ( this->_subtree != ST_CATALOG ) { return std::nullopt; }
  return this->_pathS[2];
}


/* -------------------------------------------------------------------------- */

  std::vector<std::string>
EvalPackage::getOutputs() const
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
EvalPackage::getOutputsToInstall() const
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
EvalPackage::getLicense() const
{
  if ( ! this->_hasMetaAttr ) { return std::nullopt; }
  MaybeCursor l = this->_cursor->getAttr( "meta" )->maybeGetAttr( "license" );
  if ( l == nullptr ) { return std::nullopt; }
  try
    {
      return l->getAttr( "spdxId" )->getString();
    }
  catch( ... )
    {
      return std::nullopt;
    }
}


/* -------------------------------------------------------------------------- */

  std::optional<bool>
EvalPackage::isBroken() const
{
  if ( ! this->_hasMetaAttr ) { return std::nullopt; }
  MaybeCursor b = this->_cursor->getAttr( "meta" )->maybeGetAttr( "broken" );
  if ( b == nullptr ) { return std::nullopt; }
  return b->getBool();
}

  std::optional<bool>
EvalPackage::isUnfree() const
{
  if ( ! this->_hasMetaAttr ) { return std::nullopt; }
  MaybeCursor u = this->_cursor->getAttr( "meta" )->maybeGetAttr( "unfree" );
  if ( u == nullptr ) { return std::nullopt; }
  return u->getBool();
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
