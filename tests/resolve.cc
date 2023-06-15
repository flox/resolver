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
  Inputs      inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs;
  ResolverState rs( inputs, prefs );
  auto ps = rs.getInputs().at( "nixpkgs" )->getActualFlakeAttrPathPrefixes();
  return ps.size() == defaultSystems.size();
}


/* -------------------------------------------------------------------------- */

/* Ensure that iterator ends on final attribute. */
  bool
test_FlakeIterator1()
{
  Inputs              inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences         prefs;
  ResolverState       rs( inputs, prefs );
  nix::ref<FloxFlake> ps = rs.getInputs().at( "nixpkgs" );
  std::vector<nix::Symbol> path = {
    rs.getEvalState()->symbols.create( "legacyPackages" )
  , rs.getEvalState()->symbols.create( "x86_64-linux" )
  , rs.getEvalState()->symbols.create( "hello" )
  , rs.getEvalState()->symbols.create( "meta" )
  };
  Cursor c = ps->openCursor( path );
  for ( FloxFlake::Iterator p = ps->beginAt( c ); p != ps->endAt( c ); ++p )
    {
      if ( p->getAttrPathStr() ==
           "legacyPackages.x86_64-linux.hello.meta.unsupported"
         )
        {
          return true;
        }
    }
  return false;
}


/* -------------------------------------------------------------------------- */

/* Ensure that iterator ends on final attribute. */
  bool
test_FlakeIterator1()
{
  Inputs              inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences         prefs;
  ResolverState       rs( inputs, prefs );
  nix::ref<FloxFlake> ps = rs.getInputs().at( "nixpkgs" );
  std::vector<nix::Symbol> path = {
    rs.getEvalState()->symbols.create( "legacyPackages" )
  , rs.getEvalState()->symbols.create( "x86_64-linux" )
  , rs.getEvalState()->symbols.create( "hello" )
  , rs.getEvalState()->symbols.create( "meta" )
  };
  Cursor c = ps->openCursor( path );
  for ( FloxFlake::Iterator p = ps->beginAt( c ); p != ps->endAt( c ); ++p )
    {
      if ( p->getAttrPathStr() ==
           "legacyPackages.x86_64-linux.hello.meta.unsupported"
         )
        {
          return true;
        }
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
  RUN_TEST( FlakeIterator1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
