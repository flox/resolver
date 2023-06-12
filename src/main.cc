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


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;


/* -------------------------------------------------------------------------- */

const std::string defaultInputs = "{"
  "\"nixpkgs\":"      "\"github:NixOS/nixpkgs\","
  "\"nixpkgs-flox\":" "\"github:flox/nixpkgs-flox\","
  "\"floxpkgs\":"     "\"github:flox/floxpkgs\""
"}";


/* -------------------------------------------------------------------------- */

  static nlohmann::json
readOrParseJSON( const std::string & i )
{
  nlohmann::json j;
  if ( std::filesystem::exists( i ) )
    {
      j = nlohmann::json::parse( std::ifstream( i ) );
    }
  else
    {
      j = nlohmann::json::parse( i );
    }
  return j;
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  argparse::ArgumentParser prog( "resolver", "0.1.0" );
  prog.add_description( "Resolve nix package descriptors in flakes" );

  prog.add_argument( "-o", "--one" )
    .default_value( false )
    .implicit_value( true )
    .help( "return single resolved entry or `null'" );

  prog.add_argument( "-q", "--quiet" )
    .default_value( false )
    .implicit_value( true )
    .help( "exit 0 even if no resolutions are found" );

  prog.add_argument( "-i", "--inputs" )
    .default_value( defaultInputs )
    .help( "inline JSON or path to JSON file containing flake references" )
    .metavar( "INPUTS" );

  prog.add_argument( "-p", "--preferences" )
    .default_value( std::string( "{}" ) )
    .help( "inline JSON or path to JSON file containing resolver preferences" )
    .metavar( "PREFERENCES" );

  prog.add_argument( "-d", "--descriptor" )
    .required()
    .help( "inline JSON or path to JSON file containing a package descriptor" )
    .metavar( "DESCRIPTOR" );

  try
    {
      prog.parse_args( argc, argv );
    }
  catch( const std::runtime_error & err )
    {
      std::cerr << err.what() << std::endl << prog;
      return EXIT_FAILURE;
    }

  bool one   = prog.get<bool>( "-o" );
  bool quiet = prog.get<bool>( "-q" );

  Inputs      inputs( readOrParseJSON( prog.get<std::string>( "-i" ) );
  Preferences prefs(  readOrParseJSON( prog.get<std::string>( "-p" ) );
  Descriptor  desc(   readOrParseJSON( prog.get<std::string>( "-d" ) );

  if ( one )
    {
      std::optional<Resolved> rsl = resolveOne( inputs, prefs, desc );
      if ( ! rsl.has_value() )
        {
          std::cout << "null" << std::endl;
          return quiet ? EXIT_SUCCESS : EXIT_FAILURE;
        }
      else
        {
          std::cout << rsl.value().toJSON().dump() << std::endl;
          return EXIT_SUCCESS;
        }
    }
  else
    {
      std::vector<Resolved> rsl = resolve( inputs, prefs, desc );
      std::cout << nlohmann::json( rsl ).dump() << std::endl;
      return quiet ? EXIT_SUCCESS : EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
