/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "test.hh"
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include "resolve.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

  bool
test_ResolverStateLocking1()
{
  Inputs      inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs;
  return ResolverState( inputs, prefs ).getInputs().at( "nixpkgs" )
           ->getLockedFlake()->flake.lockedRef.input.isLocked();
}


/* -------------------------------------------------------------------------- */

  bool
test_getActualFlakeAttrPathPrefixes()
{
  Inputs        inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences   prefs;
  ResolverState rs( inputs, prefs );
  auto ps = rs.getInputs().at( "nixpkgs" )->getActualFlakeAttrPathPrefixes();
  return ps.size() == defaultSystems.size();
}


/* -------------------------------------------------------------------------- */

/* Use relative path. */
  bool
test_resolveInInput1()
{
  Inputs        inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences   prefs;
  ResolverState rs( inputs, prefs );
  Descriptor    desc( (nlohmann::json) { { "path", { "hello" } } } );
  std::list<Resolved> results = rs.resolveInInput( "nixpkgs", desc );
  return results.size() == 1;
}


/* -------------------------------------------------------------------------- */

/* Ensure name resolution works. */
  bool
test_resolveInInput2()
{
  Inputs        inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences   prefs;
  ResolverState rs( inputs, prefs );
  Descriptor    desc( (nlohmann::json) { { "name", "hello" } } );
  std::list<Resolved> results = rs.resolveInInput( "nixpkgs", desc );
  if ( results.empty() ) { return false; }
  for ( const nlohmann::json & i : results.front().info )
    {
      return i["pname"] == "hello";
    }
  return false;
}


/* -------------------------------------------------------------------------- */

/* Ensure name resolution works. */
  bool
test_resolve_V2_1()
{
  Inputs        inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences   prefs;
  ResolverState rs( inputs, prefs );
  Descriptor    desc( (nlohmann::json) { { "name", "hello" } } );
  std::list<Resolved> results = resolve_V2( rs, desc );
  if ( results.empty() ) { return false; }
  for ( const nlohmann::json & i : results.front().info )
    {
      return i["pname"] == "hello";
    }
  return false;
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

  RUN_TEST( ResolverStateLocking1 );
  RUN_TEST( getActualFlakeAttrPathPrefixes );
  RUN_TEST( resolveInInput1 );
  RUN_TEST( resolveInInput2 );
  RUN_TEST( resolve_V2_1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
