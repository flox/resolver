/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>
#include <iostream>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include "resolve.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

static const std::string nixpkgsRef =
  "github:NixOS/nixpkgs/e8039594435c68eb4f780f3e9bf3972a7399c4b1";


/* -------------------------------------------------------------------------- */

  bool
test_resolve1()
{
  Inputs      inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs;
  Descriptor  desc( (nlohmann::json) { { "name", "hello" } } );

  std::vector<Resolved> rsl = resolve( inputs, prefs, desc );

  return rsl.size() == 1;
}


/* -------------------------------------------------------------------------- */

  bool
test_resolve2()
{
  Inputs      inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs;
  Descriptor  desc( (nlohmann::json) {
    { "name", "nodejs" }, { "semver", ">=14" }
  } );
  std::vector<Resolved> rsl = resolve( inputs, prefs, desc );
  return rsl.size() == 10;
}


/* -------------------------------------------------------------------------- */

  bool
test_resolveOne1()
{
  Inputs      inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs;
  Descriptor  desc( (nlohmann::json) { { "name", "hello" } } );

  std::vector<Resolved>   rsl = resolve( inputs, prefs, desc );
  std::optional<Resolved> one = resolveOne( inputs, prefs, desc );

  return ( rsl.size() == 1 ) &&
         one.has_value() &&
         ( one.value().toJSON() == rsl[0].toJSON() );
}


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

#define RUN_TEST( _NAME )                                              \
  try                                                                  \
    {                                                                  \
      if ( ! test_ ## _NAME () )                                       \
        {                                                              \
          ec = EXIT_FAILURE;                                           \
          std::cerr << "  fail: " # _NAME << std::endl;                \
        }                                                              \
    }                                                                  \
  catch( std::exception & e )                                          \
    {                                                                  \
      ec = EXIT_FAILURE;                                               \
      std::cerr << "  ERROR: " # _NAME ": " << e.what() << std::endl;  \
    }


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;

  RUN_TEST( resolve1 );
  RUN_TEST( resolve2 );
  RUN_TEST( resolveOne1 );
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
