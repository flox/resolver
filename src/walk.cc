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
#include <nix/command.hh>
#include <string>
#include <nlohmann/json.hpp>
#include "resolve.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  static inline bool
isPkgsSubtree( std::string_view attrName )
{
  return ( attrName == "packages" ) ||
         ( attrName == "legacyPackages" ) ||
         ( attrName == "catalog"  );
}


/* -------------------------------------------------------------------------- */

static const std::unordered_set<std::string> defaultSystems = {
 "x86_64-linux", "aarch64-linux", "x86_64-darwin", "aarch64-darwin"
};

  static inline bool
shouldSearchSystem( std::string_view system )
{
  std::string s( system );
  return defaultSystems.find( s ) != defaultSystems.end();
}


/* -------------------------------------------------------------------------- */

  std::optional<bool>
isAbsAttrPathJSON( const nlohmann::json & j )
{
  if ( ! j.is_array() )
    {
      throw DescriptorException(
        "Descriptor `path' field must be lists of strings or null."
      );
    }

  std::vector<nlohmann::json> path = j;
  if ( path.empty() )
    {
      return std::nullopt;
    }

  if ( path[0].is_null() )
    {
      throw DescriptorException(
        "Descriptor `path' field may only contain `null' as its second member."
      );
    }

  return isPkgsSubtree( path[0].get<std::string_view>() );
}


  std::optional<bool>
isAbsAttrPath( const std::vector<attr_part> & path )
{
  if ( path.empty() )
    {
      return std::nullopt;
    }

  if ( std::holds_alternative<std::string>( path[0] ) )
    {
      return isPkgsSubtree( std::get<std::string>( path[0] ) );
    }
  throw DescriptorException(
    "Descriptor `path' field may only contain `null' as its second member."
  );
  return false;
}


/* -------------------------------------------------------------------------- */

  bool
isMatchingAttrPathPrefix( const std::vector<attr_part>        & prefix
                        , const std::vector<nix::SymbolStr>   & path
                        )
{
  if ( prefix.empty() ) { return true; }
  if ( path.empty() )   { return false; }

  size_t j = 0;

  /* If we have a relative prefix, make `path' relative.
   * Terminate early if relative form of `path' is too short. */
  std::optional<bool> isAbsPrefix = isAbsAttrPath( prefix );
  bool                isAbsPath   = isPkgsSubtree( path[0] );
  if ( isAbsPrefix.has_value() && ( isAbsPrefix.value() != isAbsPath ) )
    {
      if ( path.size() < ( prefix.size() + 2 ) ) { return false; }
      j = 2;
    }
  else if ( path.size() < prefix.size() )
    {
      return false;
    }

  /* Check for equality of elements. */
  for ( size_t i = 0; i < prefix.size(); ++i, ++j )
    {
      if ( std::holds_alternative<std::nullptr_t>( prefix[i] ) )
        {
          if ( ! shouldSearchSystem( path[j] ) ) { return false; }
        }
      else
        {
          std::string_view a = path[j];
          if ( std::get<std::string>( prefix[i] ) != a ) { return false; }
        }
    }
  return true;
}


  bool
isMatchingAttrPath( const std::vector<attr_part>        & prefix
                  , const std::vector<nix::SymbolStr>   & path
                  )
{

  if ( prefix.empty() ) { return true; }
  if ( path.empty() )   { return false; }

  /* If we have a relative prefix, make `path' relative. */
  std::optional<bool> isAbsPrefix = isAbsAttrPath( prefix );
  bool                isAbsPath   = isPkgsSubtree( path[0] );
  /* Terminate early if sizes differ. */
  if ( ( isAbsPrefix.has_value() && ( isAbsPrefix.value() != isAbsPath ) &&
         ( path.size() != ( prefix.size() + 2 ) )
       ) || ( prefix.size() != path.size() )
     )
    {
      return false;
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

  return true;
}


/* -------------------------------------------------------------------------- */

  bool
DescriptorFunctor::packagePredicate( const nix::eval_cache::AttrCursor & pos
                                   , const std::vector<nix::Symbol>    & path
                                   )
{
  return false;
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
