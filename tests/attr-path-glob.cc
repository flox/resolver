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
test_fromStringView1()
{
  AttrPathGlob p( { "packages", "{{system}}", "hello" } );
  return std::holds_alternative<std::nullptr_t>( p.path[1] ) &&
         ( std::get<std::nullptr_t>( p.path[1] ) == nullptr );
}


/* -------------------------------------------------------------------------- */

  bool
test_hash1()
{
  AttrPathGlob p1( { "packages", nullptr, "hello" } );
  AttrPathGlob p2( { "packages", "x86_64-linux", "hello" } );
  return std::hash<AttrPathGlob>{}( p1 ) == std::hash<AttrPathGlob>{}( p2 );
}


/* -------------------------------------------------------------------------- */

/* `true' for glob path to a package. */
  bool
test_isAbsAttrPath1()
{
  return AttrPathGlob( { "packages", nullptr, "hello" } ).isAbsolute();
}


/* -------------------------------------------------------------------------- */

/* `true' for path starting with recognized `pkgsSubtree' prefix
 * and a glob. */
  bool
test_isAbsAttrPath2()
{
  return AttrPathGlob( { "packages", nullptr } ).isAbsolute();
}


/* -------------------------------------------------------------------------- */

/* `true' for path starting with recognized `pkgsSubtree' prefix. */
  bool
test_isAbsAttrPath3()
{
  return AttrPathGlob( { "packages" } ).isAbsolute();
}


/* -------------------------------------------------------------------------- */

/* `false' for empty path. */
  bool
test_isAbsAttrPath4()
{
  return ! AttrPathGlob().isAbsolute();
}


/* -------------------------------------------------------------------------- */

/* `false' for path missing recognized prefix. */
  bool
test_isAbsAttrPath5()
{
  return ! AttrPathGlob( { "hello" } ).isAbsolute();
}


/* -------------------------------------------------------------------------- */

/* As above but using JSON. */
  bool
test_isAbsAttrPathJSON1()
{
  nlohmann::json path = R"(["hello"])"_json;
  bool           rsl  = ! AttrPathGlob::fromJSON( path ).isAbsolute();

  path =  R"(["packages",null,"hello"])"_json;
  rsl  &= AttrPathGlob::fromJSON( path ).isAbsolute();

  path =  R"(["packages",null])"_json;
  rsl  &= AttrPathGlob::fromJSON( path ).isAbsolute();

  path =  R"(["packages"])"_json;
  rsl  &= AttrPathGlob::fromJSON( path ).isAbsolute();

  path =  R"([])"_json;
  rsl  &= ! AttrPathGlob::fromJSON( path ).isAbsolute();

  return rsl;
}


/* -------------------------------------------------------------------------- */

  bool
test_coerceRelative1()
{
  AttrPathGlob path( { "packages", nullptr, "hello" } );
  path.coerceRelative();
  return path.path.size() == 1;
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
  RUN_TEST( fromStringView1 );
  RUN_TEST( hash1 );
  RUN_TEST( isAbsAttrPath1 );
  RUN_TEST( isAbsAttrPath2 );
  RUN_TEST( isAbsAttrPath3 );
  RUN_TEST( isAbsAttrPath4 );
  RUN_TEST( isAbsAttrPath5 );
  RUN_TEST( isAbsAttrPathJSON1 );
  RUN_TEST( coerceRelative1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
