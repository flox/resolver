/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <variant>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "descriptor.hh"
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

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
  if ( auto search = this->results.find( pg ); search != this->results.end() )
    {
      search->second.info.at( "systems" ).push_back( path[1] );
    }
  else
    {
      Resolved r( ref, std::move( pg ), (nlohmann::json) {
        { "name",    name }
      , { "version", version }
      , { "systems", { path[1] } }
      } );
      this->results.emplace( pg, std::move( r ) );
    }
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
