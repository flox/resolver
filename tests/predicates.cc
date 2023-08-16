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
  Inputs      inputs( nlohmann::json { { "nixpkgs", nixpkgsRef } } );
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
  Inputs      inputs( nlohmann::json { { "nixpkgs", nixpkgsRef } } );
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
  Inputs      inputs( nlohmann::json { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs( nlohmann::json {
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

/* A store path that is cached should succeed.
 * XXX: This is from `github:NixOS/nixpkgs/23.05' and may need an update in
 * the future.
 */
  bool
test_isSubstitutable1()
{
  return isSubstitutable(
    "/nix/store/snxcrrlmxw0nd0na93xs8qgbdi0fsm6z-fzf-0.40.0"
  );
}


/* -------------------------------------------------------------------------- */

/* Derivations should fail.
 * XXX: This is from `github:NixOS/nixpkgs/23.05' and may need an update in
 * the future.
 */
  bool
test_isSubstitutable2()
{
  return ! isSubstitutable(
    "/nix/store/1jxvsy1rpips2cwgjjnbd49gn8nrj4ik-fzf-0.40.0.drv"
  );
}


/* -------------------------------------------------------------------------- */

  int
main()
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

  nix::setStackSize( 64 * 1024 * 1024 );
  nix::initNix();
  nix::initGC();

  RUN_TEST( predicates1 );
  RUN_TEST( predicates2 );
  RUN_TEST( Preferences_pred );

  RUN_TEST( isSubstitutable1 );
  RUN_TEST( isSubstitutable2 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
