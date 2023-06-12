/* ========================================================================== *
 *
 * Walk attribute sets to collect satisfactory packages.
 *
 * Catalog Attrset Structure:
 *   <flake>.catalog.<system>.<stability !>.<pname>.<version !>
 *
 *
 * ! :: Attribute set also contains extra fields:
 *   - `recurseForDerivations'  ( bool )
 *   - `latest' ( alias of latest version under catalog `<pname>' attrs )
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/fetchers.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include <nix/names.hh>
#include <nix/command.hh>
#include <string>
#include <nlohmann/json.hpp>
#include "resolve.hh"
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  bool
isMatchingAttrPathPrefix( const AttrPathGlob                & prefix
                        , const std::vector<nix::SymbolStr> & path
                        )
{
  if ( prefix.path.empty() ) { return true; }
  if ( path.empty() )   { return false; }

  size_t j = 0;

  /* If we have a relative prefix, make `path' relative.
   * Terminate early if relative form of `path' is too short. */
  bool isAbsPrefix = prefix.isAbsolute();
  bool isAbsPath   = isPkgsSubtree( path[0] );
  if ( isAbsPrefix != isAbsPath )
    {
      if ( path.size() < ( prefix.path.size() + 2 ) ) { return false; }
      j = 2;
    }
  else if ( path.size() < prefix.path.size() )
    {
      return false;
    }

  /* Check for equality of elements. */
  for ( size_t i = 0; i < prefix.path.size(); ++i, ++j )
    {
      if ( std::holds_alternative<std::nullptr_t>( prefix.path[i] ) )
        {
          if ( ! shouldSearchSystem( path[j] ) ) { return false; }
        }
      else
        {
          std::string_view a = path[j];
          if ( std::get<std::string>( prefix.path[i] ) != a ) { return false; }
        }
    }
  return true;
}


  bool
isMatchingAttrPath( const AttrPathGlob                & prefix
                  , const std::vector<nix::SymbolStr> & path
                  )
{
  if ( prefix.path.empty() ) { return true; }
  if ( path.empty() )   { return false; }

  /* If we have a relative prefix, treat `path' as relative. */
  if ( ! prefix.isAbsolute() )
    {
      /* Terminate early if sizes differ. */
      if ( ( prefix.path.size() + 2 ) != path.size() ) { return false; }
    }
  else
    {
      /* Terminate early if sizes differ. */
      if ( prefix.path.size() != path.size() ) { return false; }
    }
  return isMatchingAttrPathPrefix( prefix, path );
}


/* -------------------------------------------------------------------------- */

  PkgNameVersion
nameVersionAt( nix::eval_cache::AttrCursor & pos )
{
  std::string    name = pos.getAttr( "name" )->getString();
  PkgNameVersion pnv  = { .name = name };

  std::shared_ptr<nix::eval_cache::AttrCursor> attr =
    pos.maybeGetAttr( "pname" );
  if ( attr != nullptr )
    {
      try { pnv.pname = attr->getString(); } catch ( ... ) {}
    }

  attr = pos.maybeGetAttr( "version" );
  if ( attr != nullptr )
    {
      try { pnv.version = attr->getString(); } catch ( ... ) {}
    }

  if ( ! ( pnv.pname.has_value() && pnv.version.has_value() ) )
    {
      nix::DrvName parsed( name );
      pnv.parsedName    = std::move( parsed.name );
      pnv.parsedVersion = std::move( parsed.version );
    }

  return pnv;
}

  std::string
PkgNameVersion::getPname()
{
 if ( this->pname.has_value() )
   {
     return this->pname.value();
   }
 else if ( this->parsedName.has_value() )
   {
     return this->parsedName.value();
   }
 else
   {
     throw DescriptorException(
       "Failed to parse derivation name: " + this->name
     );
   }
}

  std::string
PkgNameVersion::getVersion()
{
 if ( this->version.has_value() )
   {
     return this->version.value();
   }
 else if ( this->parsedVersion.has_value() )
   {
     return this->parsedVersion.value();
   }
 else
   {
     throw DescriptorException(
       "Failed to parse derivation version: " + this->name
     );
   }
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
