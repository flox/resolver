/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <filesystem>
#include <stddef.h>
#include <fstream>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <nix/shared.hh>
#include <nix/eval.hh>
#include <nix/eval-inline.hh>
#include <nix/flake/flake.hh>
#include <nix/store-api.hh>
#include "resolve.hh"
#include <argparse/argparse.hpp>
#include <cassert>


/* -------------------------------------------------------------------------- */

  static nlohmann::json
parseAndResolveRef( nix::EvalState & state, const char * arg )
{

  bool isJSONArg = strchr( arg, '{' ) != nullptr;

  nlohmann::json rawInput =
    isJSONArg ? nlohmann::json::parse( arg ) : arg;

  try
    {
      nix::FlakeRef originalRef =
        isJSONArg ? nix::FlakeRef::fromAttrs(
                      nix::fetchers::jsonToAttrs( rawInput )
                    )
                  : nix::parseFlakeRef( arg, nix::absPath( "." ) );

      nix::FlakeRef resolvedRef = originalRef.resolve( state.store );

      return nlohmann::json {
        { "input", std::move( rawInput ) }
      , { "originalRef", {
          { "string", originalRef.to_string() }
        , { "attrs",  nix::fetchers::attrsToJSON( originalRef.toAttrs() ) }
        } }
      , { "resolvedRef", nlohmann::json {
          { "string", resolvedRef.to_string() }
        , { "attrs",  nix::fetchers::attrsToJSON( resolvedRef.toAttrs() ) }
        } }
      };

    }
  catch( std::exception & e )
    {
      std::cerr << e.what() << std::endl;
      exit( EXIT_FAILURE );
    }

  /* Unreachable */
  assert( false );
  return nlohmann::json();

}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  nix::initNix();
  nix::initGC();

  nix::evalSettings.pureEval = false;

  nix::EvalState state( {}, nix::openStore() );

  nlohmann::json j = parseAndResolveRef( state, argv[1] );

  std::cout << j.dump() << std::endl;

  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
