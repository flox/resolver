/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>
#include <iostream>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/fetchers.hh>
#include "flox/predicates.hh"
#include "resolve.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace flox::resolve::predicates;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

static const std::string nixpkgsRef =
  "github:NixOS/nixpkgs/e8039594435c68eb4f780f3e9bf3972a7399c4b1";


/* -------------------------------------------------------------------------- */

void dummy( nix::SymbolTable * ) {}

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

  Package pkg( n->openCursor( path ), & s->symbols, false );

  PkgPred prn( hasName( "hello" ) );
  PkgPred prv( hasVersion( "2.12.0" ) );
  PkgPred pra = prn && prv;

  return pra( pkg ) && prv( pkg ) && pra( pkg );
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

  nix::initNix();
  nix::initGC();

  test_predicates1();
  // RUN_TEST( predicates1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
