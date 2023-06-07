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
#include <optional>


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

/* Conclusive `true' for glob path to a package. */
  bool
test_isAbsAttrPath1()
{
  std::vector<attr_part> path = { "packages", nullptr, "hello" };
  std::optional<bool> rsl = isAbsAttrPath( path );
  return rsl.has_value() && rsl.value();
}


/* -------------------------------------------------------------------------- */

/* Conclusive `true' for path starting with recognized `pkgsSubtree' prefix
 * and a glob. */
  bool
test_isAbsAttrPath2()
{
  std::vector<attr_part> path = { "packages", nullptr };
  std::optional<bool> rsl = isAbsAttrPath( path );
  return rsl.has_value() && rsl.value();
}


/* -------------------------------------------------------------------------- */

/* Conclusive `true' for path starting with recognized `pkgsSubtree' prefix. */
  bool
test_isAbsAttrPath3()
{
  std::vector<attr_part> path = { "packages" };
  std::optional<bool> rsl = isAbsAttrPath( path );
  return rsl.has_value() && rsl.value();
}


/* -------------------------------------------------------------------------- */

/* Inconclusive for empty path. */
  bool
test_isAbsAttrPath4()
{
  std::vector<attr_part> path = {};
  std::optional<bool> rsl = isAbsAttrPath( path );
  return ! rsl.has_value();
}


/* -------------------------------------------------------------------------- */

/* Conclusive `false' for path missing recognized prefix. */
  bool
test_isAbsAttrPath5()
{
  std::vector<attr_part> path = { "hello" };
  std::optional<bool> rsl = isAbsAttrPath( path );
  return rsl.has_value() && ( ! rsl.value() );
}


/* -------------------------------------------------------------------------- */

/* As above but using JSON. */
  bool
test_isAbsAttrPathJSON1()
{
  nlohmann::json      path = R"(["hello"])"_json;
  std::optional<bool> opt  = isAbsAttrPathJSON( path );
  bool                rsl  = opt.has_value() && ( ! opt.value() );

  path =  R"(["packages",null,"hello"])"_json;
  opt  =  isAbsAttrPathJSON( path );
  rsl  &= opt.has_value() && opt.value();

  path =  R"(["packages",null])"_json;
  opt  =  isAbsAttrPathJSON( path );
  rsl  &= opt.has_value() && opt.value();

  path =  R"(["packages"])"_json;
  opt  =  isAbsAttrPathJSON( path );
  rsl  &= opt.has_value() && opt.value();

  path =  R"([])"_json;
  opt  =  isAbsAttrPathJSON( path );
  rsl  &= ! opt.has_value();

  return rsl;
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


  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;
  RUN_TEST( isAbsAttrPath1 );
  RUN_TEST( isAbsAttrPath2 );
  RUN_TEST( isAbsAttrPath3 );
  RUN_TEST( isAbsAttrPath4 );
  RUN_TEST( isAbsAttrPath5 );
  RUN_TEST( isAbsAttrPathJSON1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
