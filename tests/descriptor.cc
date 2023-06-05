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
test_DescriptorFromJSON()
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

  bool
test_DescriptorToJSON()
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

  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;
  try
    {
      if ( ! test_DescriptorFromJSON() )
        {
          ec = EXIT_FAILURE;
          std::cerr << "  fail: DescriptorFromJSON" << std::endl;
        }
    }
  catch( std::exception & e )
    {
      std::cerr << "  ERROR: DescriptorFromJSON: " << e.what() << std::endl;
      ec = EXIT_FAILURE;
    }

  try
    {
      if ( ! test_DescriptorToJSON() )
        {
          ec = EXIT_FAILURE;
          std::cerr << "  fail: DescriptorToJSON" << std::endl;
        }
    }
  catch( std::exception & e )
    {
      std::cerr << "  ERROR: DescriptorToJSON: " << e.what() << std::endl;
      ec = EXIT_FAILURE;
    }

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
