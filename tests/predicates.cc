/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "test.hh"
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/fetchers.hh>
#include "flox/predicates.hh"
#include "resolve.hh"
#include "flox/flake-package.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace flox::resolve::predicates;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

  bool
test_predicates1()
{
  Inputs      inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs;
  ResolverState rs( inputs, prefs );

  nix::ref<FloxFlake>      n = rs.getInputs().at( "nixpkgs" );
  nix::ref<nix::EvalState> s = rs.getEvalState();

  std::vector<nix::Symbol> path = {
    s->symbols.create( "legacyPackages" )
  , s->symbols.create( "x86_64-linux" )
  , s->symbols.create( "hello" )
  };

  FlakePackage pkg( n->openCursor( path ), & s->symbols, false );

  PkgPred prn( hasName( "hello" ) );
  PkgPred prv( hasVersion( "2.12.1" ) );
  PkgPred pra = prn && prv;

  return prn( pkg ) && prv( pkg ) && pra( pkg );
}


/* -------------------------------------------------------------------------- */

  bool
test_predicates2()
{
  Inputs      inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs;
  ResolverState rs( inputs, prefs );

  nix::ref<FloxFlake>      n = rs.getInputs().at( "nixpkgs" );
  nix::ref<nix::EvalState> s = rs.getEvalState();

  std::vector<nix::Symbol> path = {
    s->symbols.create( "legacyPackages" )
  , s->symbols.create( "x86_64-linux" )
  , s->symbols.create( "hello" )
  };

  FlakePackage pkg( n->openCursor( path ), & s->symbols, false );

  PkgPred prs( hasSubtree( "legacyPackages" ) );
  PkgPred pre( hasSubtree( ST_LEGACY ) );
  PkgPred pre2 = ! PkgPred( hasSubtree( ST_CATALOG ) );
  PkgPred pra  = prs && pre && pre2;

  return prs( pkg ) && pre( pkg ) && pre2( pkg ) && pra( pkg );
}


/* -------------------------------------------------------------------------- */

/* Assert that an unfree package is properly handled. */
  bool
test_Preferences_pred()
{
  Inputs      inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs( (nlohmann::json) {
    { "allow", { { "unfree", false } } }
  } );
  ResolverState rs( inputs, prefs );

  nix::ref<FloxFlake>      n = rs.getInputs().at( "nixpkgs" );
  nix::ref<nix::EvalState> s = rs.getEvalState();

  std::vector<nix::Symbol> path = {
    s->symbols.create( "legacyPackages" )
  , s->symbols.create( "x86_64-linux" )
  , s->symbols.create( "LAStools" )
  };

  FlakePackage pkg( n->openCursor( path ), & s->symbols, false );

  return ! prefs.pred_V2()( pkg );
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

  nix::setStackSize( 64 * 1024 * 1024 );
  nix::initNix();
  nix::initGC();

  RUN_TEST( predicates1 );
  RUN_TEST( predicates2 );
  RUN_TEST( Preferences_pred );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
