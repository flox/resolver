/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>
#include <iostream>
#include <nlohmann/json.hpp>
#include "descriptor.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

  bool
test_DescriptorFromJSON1()
{
  Descriptor d( (nlohmann::json) {
    { "name",    "hello" }
  , { "flake",   true    }
  , { "catalog", false   }
  } );
  return d.searchFlakes && ( ! d.searchCatalogs ) && ( d.name == "hello" );
}


/* -------------------------------------------------------------------------- */

/* Expect error if `null' appears in path in any position other than 2nd. */
  bool
test_DescriptorFromJSON2() {
  try
    {
      Descriptor d(
        (nlohmann::json) { { "path", { "foo", "bar", nullptr } } }
      );
    }
  catch( ... )
    {
      return true;
    }
  return false;
}


/* -------------------------------------------------------------------------- */

/* Expect error if flakes and catalogs cannot be searched. */
  bool
test_DescriptorFromJSON3()
{
  try
    {
      Descriptor d(
        (nlohmann::json) { { "flake", false }, { "catalog", false } }
      );
    }
  catch( ... )
    {
      return true;
    }
  return false;
}


/* -------------------------------------------------------------------------- */

/* Expect error if flakes are allowed when `catalog.stability' is set. */
  bool
test_DescriptorFromJSON4()
{
  try
    {
      Descriptor d( (nlohmann::json) {
        { "flake", true }, { "catalog", { { "stability", "stable" } } }
      } );
    }
  catch( ... )
    {
      return true;
    }
  return false;
}


/* -------------------------------------------------------------------------- */

  bool
test_DescriptorToJSON1()
{
  Descriptor d( (nlohmann::json) {
    { "name", "hello" }, { "flake", true }, { "catalog", false }
  } );
  nlohmann::json j = d.toJSON();

  return ( j["name"].get<std::string>() == "hello" ) &&
         j["flake"].get<bool>() &&
         ( ! j["catalog"].get<bool>() );
}


/* -------------------------------------------------------------------------- */

  bool
test_DescriptorToJSON2()
{
  Descriptor d( (nlohmann::json) {
    { "name",    "hello" }
  , { "flake",   true    }
  , { "catalog", true    }
  , { "input",   "foo"   }
  } );
  nlohmann::json j = d.toJSON();

  return ( j["input"].get<std::string_view>() == "foo" ) &&
         j["flake"].get<bool>() &&
         j["catalog"].get<bool>();
}


/* -------------------------------------------------------------------------- */

  bool
test_DescriptorToString1()
{
  Descriptor d( (nlohmann::json) {
    { "name",  "hello" }
  , { "input", "foo"   }
  } );
  return d.toString() == "foo#?name=\"hello\"";
}


/* -------------------------------------------------------------------------- */

/* Make sure `catalog=false' is omitted when we have an absolute path. */
  bool
test_DescriptorToString2()
{
  Descriptor d( (nlohmann::json) {
    { "path",    { "legacyPackages", nullptr, "hello" } }
  , { "input",   "foo"                                  }
  , { "catalog", false                                  }
  , { "semver",  "^2.12.1"                              }
  } );
  return d.toString() ==
         "foo#legacyPackages.{{system}}.hello@^2.12.1";
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
  RUN_TEST( DescriptorFromJSON1 );
  RUN_TEST( DescriptorFromJSON2 );
  RUN_TEST( DescriptorFromJSON3 );
  RUN_TEST( DescriptorFromJSON4 );
  RUN_TEST( DescriptorToJSON1 );
  RUN_TEST( DescriptorToJSON2 );
  RUN_TEST( DescriptorToString1 );
  RUN_TEST( DescriptorToString2 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
