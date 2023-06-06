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
  nlohmann::json desc = R"(
    {
      "name":    "hello"
    , "flake":   true
    , "catalog": false
    }
  )"_json;
  Descriptor d( desc );
  return d.searchFlakes && ( ! d.searchCatalogs ) && ( d.name == "hello" );
}


/* -------------------------------------------------------------------------- */

/* Expect error if `null' appears in path in any position other than 2nd. */
  bool
test_DescriptorFromJSON2()
{
  nlohmann::json desc = R"(
    {
      "path": ["foo", "bar", null]
    }
  )"_json;
  bool rsl = false;
  try { Descriptor d( desc ); } catch( ... ) { rsl = true; }
  return rsl;
}


/* -------------------------------------------------------------------------- */

/* Expect error if flakes and catalogs cannot be searched. */
  bool
test_DescriptorFromJSON3()
{
  nlohmann::json desc = R"(
    {
      "flake":   false
    , "catalog": false
    }
  )"_json;
  bool rsl = false;
  try { Descriptor d( desc ); } catch( ... ) { rsl = true; }
  return rsl;
}


/* -------------------------------------------------------------------------- */

/* Expect error if flakes are allowed when `catalog.stability' is set. */
  bool
test_DescriptorFromJSON4()
{
  nlohmann::json desc = R"(
    {
      "flake":   true
    , "catalog": { "stability": "stable" }
    }
  )"_json;
  bool rsl = false;
  try { Descriptor d( desc ); } catch( ... ) { rsl = true; }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  bool
test_DescriptorToJSON1()
{
  nlohmann::json desc = R"(
    {
      "name":    "hello"
    , "flake":   true
    , "catalog": false
    }
  )"_json;
  Descriptor d( desc );
  nlohmann::json j = d.toJSON();

  return ( j["name"].get<std::string>() == "hello" ) &&
         j["flake"].get<bool>() &&
         ( ! j["catalog"].get<bool>() );
}


/* -------------------------------------------------------------------------- */

  bool
test_DescriptorToJSON2()
{
  nlohmann::json desc = R"(
    {
      "name":    "hello"
    , "flake":   true
    , "catalog": true
    , "input":   "foo"
    }
  )"_json;
  Descriptor d( desc );
  nlohmann::json j = d.toJSON();

  return ( j["input"].get<std::string_view>() == "foo" ) &&
         j["flake"].get<bool>() &&
         j["catalog"].get<bool>();
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
  RUN_TEST( DescriptorFromJSON1 );
  RUN_TEST( DescriptorFromJSON2 );
  RUN_TEST( DescriptorFromJSON3 );
  RUN_TEST( DescriptorFromJSON4 );
  RUN_TEST( DescriptorToJSON1 );
  RUN_TEST( DescriptorToJSON2 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
