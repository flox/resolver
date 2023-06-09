/* ========================================================================== *
 *
 * Walk attribute sets to collect satisfactory packages.
 *
 * Catalog Attrset Structure:
 *   <flake>.catalog.<system>.<stability !>.<pname>.<version !>
 *
 *
 * ! :: Attribute set also contains extra fields:
 *   - `recurseForDerivation'  ( bool )
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

  static inline bool
shouldSearchSystem( std::string_view system )
{
  std::string s( system );
  return defaultSystems.find( s ) != defaultSystems.end();
}


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

  bool
DescriptorFunctor::shouldRecur(       nix::eval_cache::AttrCursor & pos
                              , const std::vector<nix::Symbol>    & path
                              )
{
  if ( path.size() < 1 ) { return true; }

  std::vector<nix::SymbolStr> pathS = this->state->symbols.resolve( path );

  /* Handle prefixes. */
  if ( path.size() == 1 )
    {
      if ( ! isPkgsSubtree( pathS[0] ) ) { return false; }
      if ( ( ! this->desc->searchCatalogs ) && ( pathS[0] == "catalog" ) )
        {
          return false;
        }
      if ( ( ! this->desc->searchFlakes ) &&
           ( ( pathS[0] == "packages" ) || ( pathS[0] == "legacyPackages" ) )
         )
        {
          return false;
        }
    }

  /* Handle systems. */
  std::string_view system = pathS[1];
  if ( path.size() == 2 ) { return shouldSearchSystem( system ); }

  /* Handle stability. */
  if ( ( path.size() == 3 ) && ( pathS[0] == "catalog" ) &&
       ( this->desc->catalogStability.has_value() ) &&
       ( this->desc->catalogStability.value() !=
         std::string_view( pathS[2] )
       )
     )
    {
      return false;
    }

  /* Do not search derivation fields. */
  if ( pos.isDerivation() ) { return false; }

  /* Handle `recurseForDerivation' field. */
  std::shared_ptr<nix::eval_cache::AttrCursor> recurseForDrv =
    pos.maybeGetAttr( "recurseForDerivation" );
  if ( ( recurseForDrv != nullptr ) )
    {
      return recurseForDrv->getBool();
    }

  return path.size() <= 2;
}


/* -------------------------------------------------------------------------- */

  PkgNameVersion
nameVersionAt( std::string_view attrName, nix::eval_cache::AttrCursor & pos )
{
  std::string  name        = pos.getAttr( "name" )->getString();
  nix::DrvName parsed( name );

  PkgNameVersion pnv = {
    .name          = name
  , .attrName      = std::string( attrName )
  , .parsedName    = parsed.name
  , .parsedVersion = parsed.version
  };

  std::shared_ptr<nix::eval_cache::AttrCursor> attr =
    pos.maybeGetAttr( "pname" );
  if ( attr != nullptr ) { pnv.pname = attr->getString(); }

  attr = pos.maybeGetAttr( "version" );
  if ( attr != nullptr ) { pnv.version = attr->getString(); }

  return pnv;
}


/* -------------------------------------------------------------------------- */

  bool
DescriptorFunctor::packagePredicate(       nix::eval_cache::AttrCursor & pos
                                   , const std::vector<nix::Symbol>    & path
                                   )
{
  std::vector<nix::SymbolStr> pathS = this->state->symbols.resolve( path );

  PkgNameVersion pnv = nameVersionAt( pathS[path.size() - 1], pos );

  if ( ! pos.isDerivation() )
    {
      throw DescriptorException(
        "`packagePredicate()' must be run on a derivation."
      );
    }

  /* Check name */
  if ( this->desc->name.has_value() )
    {
      if ( ! ( ( this->desc->name == pnv.name ) ||
               ( this->desc->name == pnv.pname ) ||
               ( this->desc->name == pnv.attrName ) ||
               ( this->desc->name == pnv.attrName ) )
         )
        {
          return false;
        }
    }

  if ( this->desc->version.has_value() )
    {
      if ( ! ( this->desc->version == pnv.version ) ) { return false; }
    }

  // TODO: semver

  if ( this->desc->absAttrPath.has_value() )
    {
      if ( ! isMatchingAttrPath( this->desc->absAttrPath.value(), pathS ) )
        {
          return false;
        }
    }
  if ( this->desc->relAttrPath.has_value() )
    {
      AttrPathGlob fuzz( this->desc->relAttrPath.value() );
      if ( ! isMatchingAttrPath( fuzz, pathS ) ) { return false; }
    }

  return true;
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
