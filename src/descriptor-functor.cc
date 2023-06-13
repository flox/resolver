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
#include "semver.hh"


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

  /* Push/pop current verbosity to suppress eval traces. */
  nix::Verbosity oldV = nix::verbosity;
  nix::verbosity = nix::lvlError;

  /* Do not search derivation fields. */
  bool isDrv = pos->isDerivation();
  if ( isDrv )
    {
      nix::verbosity = oldV;
      return false;
    }

  /* Handle `recurseForDerivations' field. */
  MaybeCursor recurseForDrvs = pos->maybeGetAttr( "recurseForDerivations" );
  nix::verbosity = oldV;
  if ( recurseForDrvs != nullptr )
    {
      return recurseForDrvs->getBool();
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
  /* Push/pop current verbosity to suppress eval traces. */
  nix::Verbosity oldV  = nix::verbosity;
  nix::verbosity = nix::lvlError;
  bool isDrv = pos->isDerivation();
  nix::verbosity = oldV;
  if ( ! isDrv )
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
      AttrPathGlob fuzz = AttrPathGlob::fromStrings(
        this->desc->relAttrPath.value()
      );
      if ( ! isMatchingAttrPath( fuzz, pathS ) ) { return false; }
    }

  /* Stability */
  if ( this->desc->catalogStability.has_value() &&
       ( pathS[2] != this->desc->catalogStability.value() )
     )
    {
      return false;
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

  std::optional<std::string> version;
  if ( this->desc->version.has_value() || this->desc->semver.has_value() )
    {
      if ( pnv.version.has_value() )
        {
          version = pnv.version.value();
        }
      else if ( pnv.parsedVersion.has_value() )
        {
          version = pnv.parsedVersion.value();
        }
      else
        {
          throw DescriptorException(
            "Failed to parse derivation version: " + pnv.name
          );
        }
    }

  /* Exact version */
  if ( this->desc->version.has_value() &&
       ( this->desc->version.value() != version.value() )
     )
    {
      return false;
    }

  /* Semver */
  if ( this->desc->semver.has_value() )
    {
      if ( ! version.has_value() ) { return false; }
      std::optional<std::string> semver = coerceSemver( version.value() );
      if ( ! semver.has_value() ) { return false; }
      std::list<std::string> sat = semverSat(
        this->desc->semver.value(), { semver.value() }
      );
      if ( sat.empty() ) { return false; }
    }

  return this->prefsPredicate( pos, path );
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

  std::optional<std::string> semver;
  if ( ! version.empty() )
    {
       semver = coerceSemver( version );
    }

  /* If this result already exists append `systems', otherwise add. */
  if ( auto search = this->results.find( pg ); search != this->results.end() )
    {
      nlohmann::json & info = search->second.info;
      info.at( "systems" ).push_back( path[1] );
      info.at( "names" ).emplace( path[1], std::string( name ) );
      if ( ! version.empty() )
        {
          if ( info.find( "versions" ) == info.end() )
            {
              info["versions"] = { { path[1], std::string( version ) } };
            }
          else
            {
              info.at( "versions" ).emplace( path[1], std::string( version ) );
            }

          if ( semver.has_value() )
            {
              if ( info.find( "semvers" ) == info.end() )
                {
                  info["semvers"] = { { path[1], semver.value() } };
                }
              else
                {
                  info.at( "semvers" ).emplace( path[1], semver.value() );
                }
            }
        }
    }
  else
    {
      /* Add a new entry. */
      Resolved r( ref, pg, (nlohmann::json) {
        { "systems", { path[1] } }
      , { "names",   { { path[1], std::string( name ) } } }
      } );
      if ( ! version.empty() )
        {
          if ( r.info.find( "versions" ) == r.info.end() )
            {
              r.info["versions"] = { { path[1], std::string( version ) } };
            }
          else
            {
              r.info.at( "versions" ).emplace( path[1]
                                             , std::string( version )
                                             );
            }
          if ( semver.has_value() )
            {
              if ( r.info.find( "semvers" ) == r.info.end() )
                {
                  r.info["semvers"] = { { path[1], semver.value() } };
                }
              else
                {
                  r.info.at( "semvers" ).emplace( path[1] , semver.value() );
                }
            }
        }
      this->results.emplace( std::move( pg ), std::move( r ) );
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
  else
    {
      /* Push/pop current verbosity to suppress eval traces. */
      nix::Verbosity oldV  = nix::verbosity;
      nix::verbosity = nix::lvlError;
      bool isDrv = cur->isDerivation();
      nix::verbosity = oldV;
      if ( isDrv && this->packagePredicate( cur, attrPath ) )
        {
          PkgNameVersion pnv = nameVersionAt( * cur );
          this->addResult( ref, attrPathS, pnv.getPname(), pnv.getVersion() );
        }
    }
}


/* -------------------------------------------------------------------------- */

  std::vector<CursorPos>
DescriptorFunctor::getRoots(
  std::string_view                         inputId
, std::shared_ptr<nix::flake::LockedFlake> flake
)
{
  const std::vector<std::string> * prefixes = & defaultAttrPathPrefixes;
  if ( auto search = this->prefs->prefixes.find( std::string( inputId ) );
       search != this->prefs->prefixes.end()
     )
    {
      prefixes = & search->second;
    }

  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( * this->state, flake );

  Cursor                 root = cache->getRoot();
  std::vector<CursorPos> roots;

  for ( auto & p : * prefixes )
    {
      if ( ( ! this->desc->searchCatalogs ) && ( p == "catalog" ) )
        {
          continue;
        }
      if ( ( ! this->desc->searchFlakes ) &&
           ( ( p == "packages" ) || ( p == "legacyPackages" ) )
         )
        {
          continue;
        }
      nix::Symbol s = state->symbols.create( p );
      MaybeCursor c = root->maybeGetAttr( s );
      if ( c != nullptr )
        {
          std::vector<nix::Symbol> path = { s };
          CursorPos r = std::make_pair( Cursor( c ), path );
          roots.push_back( r );
        }
    }
  return roots;
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
