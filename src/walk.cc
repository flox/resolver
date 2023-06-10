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
DescriptorFunctor::shouldRecur(
        nix::ref<nix::eval_cache::AttrCursor>   pos
, const std::vector<nix::Symbol>              & path
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
  if ( pos->isDerivation() ) { return false; }

  /* Handle `recurseForDerivation' field. */
  std::shared_ptr<nix::eval_cache::AttrCursor> recurseForDrv =
    pos->maybeGetAttr( "recurseForDerivation" );
  if ( ( recurseForDrv != nullptr ) )
    {
      return recurseForDrv->getBool();
    }

  return path.size() <= 2;
}


/* -------------------------------------------------------------------------- */

  PkgNameVersion
nameVersionAt( nix::eval_cache::AttrCursor & pos )
{
  std::string    name = pos.getAttr( "name" )->getString();
  PkgNameVersion pnv  = { .name = name };

  std::shared_ptr<nix::eval_cache::AttrCursor> attr =
    pos.maybeGetAttr( "pname" );
  if ( attr != nullptr ) { pnv.pname = attr->getString(); }

  attr = pos.maybeGetAttr( "version" );
  if ( attr != nullptr ) { pnv.version = attr->getString(); }

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

  bool
DescriptorFunctor::packagePredicate(
        nix::ref<nix::eval_cache::AttrCursor>   pos
, const std::vector<nix::Symbol>              & path
)
{
  if ( ! pos->isDerivation() )
    {
      throw DescriptorException(
        "`packagePredicate()' must be run on a derivation."
      );
    }

  std::vector<nix::SymbolStr> pathS = this->state->symbols.resolve( path );

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

  PkgNameVersion pnv = nameVersionAt( * pos );

  /* Check name */
  if ( this->desc->name.has_value() )
    {
      std::string_view n = this->desc->name.value();
      std::string_view o;
      if ( pnv.pname.has_value() )           { o = pnv.pname.value(); }
      else if ( pnv.parsedName.has_value() ) { o = pnv.parsedName.value(); }
      else
        {
          throw DescriptorException(
            "Failed to parse derivation name: " + pnv.name
          );
        }
      if ( ! ( ( n == pathS[path.size() - 1] ) ||
               ( n == pnv.name ) ||
               ( n == o )
             )
         )
        {
          return false;
        }
    }

  if ( this->desc->version.has_value() )
    {
      std::string_view o;
      if ( pnv.version.has_value() )
        {
          o = pnv.version.value();
        }
      else if ( pnv.parsedVersion.has_value() )
        {
          o = pnv.parsedVersion.value();
        }
      else
        {
          throw DescriptorException(
            "Failed to parse derivation version: " + pnv.name
          );
        }
      if ( this->desc->version.value() != o ) { return false; }
    }

  // TODO: semver

  return true;
}


/* -------------------------------------------------------------------------- */

  void
DescriptorFunctor::addResult( const FloxFlakeRef                & ref
                            , const std::vector<nix::SymbolStr> & path
                            ,       std::string_view              name
                            ,       std::string_view              version
                            )
{
  AttrPathGlob pg;
  pg.path.push_back( path[0] );
  pg.path.push_back( nullptr );
  for ( size_t i = 2; i < path.size(); ++i )
    {
      pg.path.push_back( path[i] );
    }
  /* If this result already exists append `systems', otherwise add. */
  for ( auto & r : this->results )
    {
      if ( r.path.globEq( pg ) )
        {
          r.info.at( "systems" ).push_back( path[1] );
          return;
        }
    }
  Resolved r( ref, std::move( pg ), (nlohmann::json) {
    { "name",    name }
  , { "version", version }
  , { "systems", { path[1] } }
  } );
  this->results.push_back( std::move( r ) );
}


/* -------------------------------------------------------------------------- */

  void
DescriptorFunctor::visit(
  const FloxFlakeRef                          & ref
,       nix::ref<nix::eval_cache::AttrCursor>   cur
, const std::vector<nix::Symbol>              & attrPath
)
{
  std::vector<nix::SymbolStr> attrPathS =
    this->state->symbols.resolve( attrPath );

  if ( this->shouldRecur( cur, attrPath ) )
    {
      for ( const auto & attr : cur->getAttrs() )
        {
          try
            {
              std::vector<nix::Symbol> attrPath2( attrPath );
              attrPath2.push_back( attr );
              nix::ref<nix::eval_cache::AttrCursor> child =
                cur->getAttr( this->state->symbols[attr] );
              visit( ref, child, attrPath2 );
            }
          catch ( nix::EvalError & e )
            {
              if ( ! ( ( attrPathS[0] == "legacyPackages" ) &&
                       ( 0 < attrPath.size() ) )
                 )
                {
                  throw;
                }
            }
        }
    }
  else if ( cur->isDerivation() && this->packagePredicate( cur, attrPath ) )
    {
      PkgNameVersion pnv = nameVersionAt( * cur );
      this->addResult( ref, attrPathS, pnv.getPname(), pnv.getVersion() );
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
