/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>
#include <iostream>
#include <nlohmann/json.hpp>
#include "resolve.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

  bool
test_InputsFromJSON1()
{
  nlohmann::json inputs = R"(
    {
      "nixpkgs":  "github:NixOS/nixpkgs"
    , "nixpkgs2": {
        "type": "github"
      , "owner": "NixOS"
      , "repo":  "nixpkgs"
      }
    }
  )"_json;
  Inputs d( inputs );
  return true;
}

  bool
test_InputsToJSON1()
{
  nlohmann::json inputs = R"(
    {
      "nixpkgs":  "github:NixOS/nixpkgs"
    , "nixpkgs2": {
        "type": "github"
      , "owner": "NixOS"
      , "repo":  "nixpkgs"
      }
    }
  )"_json;
  Inputs d( inputs );
  nlohmann::json j = d.toJSON();
  return j["nixpkgs"] == j["nixpkgs2"];
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
  RUN_TEST( InputsFromJSON1 );
  RUN_TEST( InputsToJSON1 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
