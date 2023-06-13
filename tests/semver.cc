/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>
#include <iostream>
#include "semver.hh"


/* -------------------------------------------------------------------------- */

  bool
test_semverSat1()
{
  std::list<std::string> sats = semverSat( "^4.2.0", {
    "4.0.0", "4.2.0", "4.2.1", "4.3.0", "5.0.0", "3.9.9"
  } );
  return ( sats.size() == 3 ) &&
         ( std::find( sats.begin(), sats.end(), "4.2.0" ) != sats.end() ) &&
         ( std::find( sats.begin(), sats.end(), "4.2.1" ) != sats.end() ) &&
         ( std::find( sats.begin(), sats.end(), "4.3.0" ) != sats.end() );
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
  RUN_TEST( semverSat1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
