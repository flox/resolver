/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

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


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  bool           quiet = false;
  bool           one   = false;
  nlohmann::json ij;
  nlohmann::json pj;
  nlohmann::json dj;
  for ( int i = 1, field = 0; ( i < argc ) && ( field < 3 ); ++i )
    {
      std::string_view arg( argv[i] );
      if ( ( arg == "--quiet" ) || ( arg == "-q" ) )
        {
          quiet = true;
        }
      if ( ( arg == "--one" ) || ( arg == "-o" ) )
        {
          one = true;
        }
      else if ( ( arg == "--file" ) || ( arg == "-f" ) )
        {
          ++i;
          if ( argc <= i )
            {
              throw ResolverException(
                "The flag '--file' requires an argument."
              );
            }
          switch ( field )
            {
              case 0:
                ij = nlohmann::json::parse( std::ifstream( argv[i] ) );
                break;
              case 1:
                pj = nlohmann::json::parse( std::ifstream( argv[i] ) );
                break;
              case 2:
                dj = nlohmann::json::parse( std::ifstream( argv[i] ) );
                break;
            }
          ++field;
        }
      else
        {
          switch ( field )
            {
              case 0:
                ij = nlohmann::json::parse( argv[i] );
                break;
              case 1:
                pj = nlohmann::json::parse( argv[i] );
                break;
              case 2:
                dj = nlohmann::json::parse( argv[i] );
                break;
            }
          ++field;
        }
    }

  Inputs      inputs( ij );
  Preferences prefs( pj );
  Descriptor  desc( dj );

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
