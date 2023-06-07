/* ========================================================================== *
 *
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

static const std::unordered_set<std::string> defaultSystems = {
 "x86_64-linux", "aarch64-linux", "x86_64-darwin", "aarch64-darwin"
};

  static inline bool
shouldSearchSystem( std::string_view system )
{
  std::string s( system );
  return defaultSystems.find( s ) != defaultSystems.end();
}


  static inline std::optional<std::string>
getSystemFromAttrPath( const nix::EvalState           & state
                     , const std::vector<nix::Symbol> & path
                     )
{
  if ( path.size() < 2 )
    {
      return std::nullopt;
    }
  std::vector<nix::SymbolStr> pathS = state.symbols.resolve( path );
  std::string s = pathS[1];

  return ( s == "{{system}}" ) ? std::nullopt
                               : std::optional<std::string> { s };
}


/* -------------------------------------------------------------------------- */

  static inline bool
isPkgsSubtree( std::string_view attrName )
{
  return ( attrName == "packages" ) || ( attrName == "legacyPackages" ) ||
         ( attrName == "catalog"  ) || ( attrName == "evalCatalog" );
}


/* -------------------------------------------------------------------------- */

  bool
DescriptorFunctor::shouldRecur(
        nix::EvalState              & state
, const Preferences                 & prefs
,       nix::eval_cache::AttrCursor & pos
, const std::vector<nix::Symbol>    & path
)
{
  if ( path.size() < 1 ) { return true; }

  std::vector<nix::SymbolStr> pathS = state.symbols.resolve( path );
  if ( path.size() == 1 )
    {
      if ( ! isPkgsSubtree( pathS[0] ) ) { return false; }
      if ( ! this->descriptor->searchCatalogs )
        {
          if ( ( pathS[0] == "catalog" ) || ( pathS[0] == "evalCatalog" ) )
            {
              return false;
            }
        }
      if ( ! this->descriptor->searchFlakes )
        {
          if ( ( pathS[0] == "packages" ) || ( pathS[0] == "legacyPackages" ) )
            {
              return false;
            }
        }
    }

  std::string_view system = pathS[1];
  if ( path.size() == 2 ) { return shouldSearchSystem( system ); }

  // TODO: check stability

  if ( pos.isDerivation() ) { return false; }

  std::shared_ptr<nix::eval_cache::AttrCursor> recurseForDrv =
    pos.maybeGetAttr( "recurseForDerivation" );
  if ( ( recurseForDrv != nullptr ) )
    {
      return recurseForDrv->getBool();
    }

  return true;
}


  bool
DescriptorFunctor::packagePredicate(
        nix::EvalState              & state
, const Preferences                 & prefs
, const nix::eval_cache::AttrCursor & pos
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
