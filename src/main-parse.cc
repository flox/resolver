/* ========================================================================== *
 *
 * Parse various URIs and junk using `nix' libraries and expose them in a
 * trivially simple way so that they can be used by any UNIX software.
 *
 * -------------------------------------------------------------------------- *
 *
 * Any `flox' team member who creates a PR which pushes this file beyone 500
 * lines or modifies this comment to raise the limit above 500 lines is
 * ethically, legally, and spiritually obliged to consume an entire shoe of
 * at least US mens size 9 or larger.
 *
 * Any team member who endures this hazing is guaranteed an unconditional
 * approval for any commit made to this file which will be preserved in
 * perpetuity until `flox' is defunct or acquired by a parent company -
 * because honestly you ate an adult sized shoe and you definitely shouldn't
 * have made this file more complicated than it strictly needed to be but I
 * guess you earned it and will assume you saved this trump card for a
 * sufficiently extreme circumstance that really justified it.
 * Also if you just copy the contents of this file into another executable to
 * make it longer and avoid eating a shoe you're a coward and you'll have to
 * live with that shame for the remainder of your natural life.
 *
 * - Alex Ameen <alex.ameen.tx@gmail.com>
 * Email me if anyone eventually eats a shoe one day, you gotta tell me their
 * rationale and obviously I'll NDA. <3
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
#include <argparse/argparse.hpp>
#include <cassert>
#include <ranges>


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

  static nlohmann::json
parseInstallable( nix::EvalState & state, const char * arg )
{
  try
    {
      std::string url( arg );
      std::tuple<nix::FlakeRef, std::string, nix::ExtendedOutputsSpec> parsed =
        nix::parseFlakeRefWithFragmentAndExtendedOutputsSpec(
          url
        , nix::absPath( "." )
        );

      nix::FlakeRef ref = std::get<0>( parsed );

      nix::ExtendedOutputsSpec exOuts  = std::get<2>( parsed );
      nlohmann::json           outputs;

      if ( std::holds_alternative<nix::OutputsSpec>( exOuts.raw() ) )
        {
          nix::OutputsSpec outSpec = std::get<nix::OutputsSpec>( exOuts.raw() );
          if ( std::holds_alternative<nix::OutputNames>( outSpec.raw() ) )
            {
              nix::OutputNames outs =
                std::get<nix::OutputNames>( outSpec.raw() );
              for ( auto & out : outs )
                {
                  outputs.push_back( std::move( out ) );
                }
            }
          else  /* All */
            {
              outputs = "all";
            }
        }
      else
        {
          outputs = "default";
        }


      return nlohmann::json {
        { "input", std::move( arg ) }
      , { "ref", {
          { "string", ref.to_string() }
        , { "attrs",  nix::fetchers::attrsToJSON( ref.toAttrs() ) }
        } }
      , { "attrPath",
          nix::tokenizeString<std::vector<std::string>>(
            std::get<1>( parsed )
          )
        }
      , { "outputs", std::move( outputs ) }
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

  char   cmd = '\0';
  char * arg = nullptr;

  nlohmann::json j;

  switch ( argc )
    {
      case 2:
        cmd = 'r';
        arg = argv[1];
        break;

      case 3:
        cmd = argv[1][1];
        arg = argv[2];
        break;

      default:
        std::cerr << "Too few arguments!" << std::endl;
        return EXIT_FAILURE;
        break;
    }

  switch ( cmd )
    {
      case 'r': j = parseAndResolveRef( state, argv[2] ); break;
      case 'i': j = parseInstallable(   state, argv[2] ); break;
      default:
        std::cerr << "Unrecognized command flag: " << argv[1] << std::endl;
        return EXIT_FAILURE;
        break;
    }


  std::cout << j.dump() << std::endl;

  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
