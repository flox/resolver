/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>
#include "flox/types.hh"
#include <functional>


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;

/* -------------------------------------------------------------------------- */

  bool
test_hash1()
{
  AttrPathGlob p1( (nlohmann::json) { "packages", nullptr, "hello" } );
  std::vector<attr_part> v = { "packages", "x86_64-linux", "hello" };
  AttrPathGlob p2( v );
  return std::hash<AttrPathGlob>{}( p1 ) == std::hash<AttrPathGlob>{}( p2 );
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
  RUN_TEST( hash1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
