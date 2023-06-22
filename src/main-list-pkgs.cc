/* ========================================================================== *
 *
 * List all derivations in a flake.
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
#include "flox/drv-cache.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;


/* -------------------------------------------------------------------------- */

/* We don't use a raw string here because it is used in the `--help' message. */
const std::string defaultInput = "github:NixOS/nixpkgs";


/* -------------------------------------------------------------------------- */

  static inline nlohmann::json
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
  argparse::ArgumentParser prog( "list-pkgs", "0.1.0" );
  prog.add_description( "List all derivations in a flake" );

  prog.add_argument( "-i", "--input" )
    .default_value( defaultInput )
    .help( "flake ref to be scraped" )
    .metavar( "INPUT" );

  prog.add_argument( "-p", "--preferences" )
    .default_value( std::string( "{}" ) )
    .help( "inline JSON or path to JSON file containing resolver preferences" )
    .metavar( "PREFERENCES" );

  prog.add_argument( "-j", "--json" )
    .default_value( false )
    .implicit_value( true )
    .help( "whether to print JSON output instead of lines" );

  try
    {
      prog.parse_args( argc, argv );
    }
  catch( const std::runtime_error & err )
    {
      std::cerr << err.what() << std::endl << prog;
      return EXIT_FAILURE;
    }

  bool json = prog.get<bool>( "-j" );

  Inputs inputs( (nlohmann::json) {
    { "target", prog.get<std::string>( "-i" ) }
  } );
  Preferences prefs( readOrParseJSON( prog.get<std::string>( "-p" ) ) );

  /* TODO: make an option */
  nix::verbosity = nix::lvlError;

  ResolverState rs( inputs, prefs );

  nix::ref<FloxFlake> flake = rs.getInput( "target" ).value();

  bool first = true;
  if ( json ) { std::cout << '['; }

  for ( const std::list<std::string> & prefix :
          flake->getFlakeAttrPathPrefixes()
      )
    {
      auto it = prefix.cbegin();
      std::advance( it, 1 );
      std::string_view system  = * it;

      std::cerr << "Processing derivations for prefix '" << prefix.front()
                << '.' << system << "': ";
      progress_status status =
        flake->populateDerivations( prefix.front(), system );
      std::cerr << "done" << std::endl;
      if ( status == DBPS_EMPTY ) { continue; }

      DrvDb db( flake->getLockedFlake()->getFingerprint() );
      std::optional<std::list<std::vector<std::string>>> _paths =
        db.getDrvPaths( prefix.front(), system );
      if ( ! _paths.has_value() ) { continue; }
      std::list<std::vector<std::string>> paths = std::move( _paths.value() );

      for ( std::vector<std::string> & path : paths )
        {
          if ( json )
            {
              if ( first ) { std::cout << ' '; first = false; }
              else { std::cout << ", "; }
              nlohmann::json p = { prefix.front(), system };
              for ( std::string & pp : path  )
                {
                  p.push_back( std::move( pp ) );
                }
              std::cout << p << std::endl;
            }
          else
            {
              std::cout << prefix.front() << "." << system << ".";
              for ( size_t i = 0; i < path.size(); ++i )
                {
                  std::cout << path.at( i );
                  if ( ( i + 1 ) < path.size() ) { std::cout << '.'; }
                }
              std::cout << std::endl;
            }
        }
    }
  if ( json ) { std::cout << ']'; }

  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
