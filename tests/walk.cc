/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cstddef>
#include <iostream>
#include <nlohmann/json.hpp>
#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/fetchers.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
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
  std::optional<bool>    rsl  = isAbsAttrPath( path );
  return rsl.has_value() && rsl.value();
}


/* -------------------------------------------------------------------------- */

/* Conclusive `true' for path starting with recognized `pkgsSubtree' prefix
 * and a glob. */
  bool
test_isAbsAttrPath2()
{
  std::vector<attr_part> path = { "packages", nullptr };
  std::optional<bool>    rsl  = isAbsAttrPath( path );
  return rsl.has_value() && rsl.value();
}


/* -------------------------------------------------------------------------- */

/* Conclusive `true' for path starting with recognized `pkgsSubtree' prefix. */
  bool
test_isAbsAttrPath3()
{
  std::vector<attr_part> path = { "packages" };
  std::optional<bool>    rsl  = isAbsAttrPath( path );
  return rsl.has_value() && rsl.value();
}


/* -------------------------------------------------------------------------- */

/* Inconclusive for empty path. */
  bool
test_isAbsAttrPath4()
{
  std::vector<attr_part> path = {};
  std::optional<bool>    rsl  = isAbsAttrPath( path );
  return ! rsl.has_value();
}


/* -------------------------------------------------------------------------- */

/* Conclusive `false' for path missing recognized prefix. */
  bool
test_isAbsAttrPath5()
{
  std::vector<attr_part> path = { "hello" };
  std::optional<bool>    rsl  = isAbsAttrPath( path );
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

/* Absolute prefix with globs should work. */
  bool
test_isMatchingAttrPathPrefix1( nix::EvalState & state )
{
  std::vector<attr_part>   prefix = { "packages", nullptr };
  std::vector<nix::Symbol> parsed;
  parsed.push_back( state.symbols.create( "packages" ) );
  parsed.push_back( state.symbols.create( "x86_64-linux" ) );
  parsed.push_back( state.symbols.create( "hello" ) );

  std::vector<nix::SymbolStr> path = state.symbols.resolve( parsed );

  return isMatchingAttrPathPrefix( prefix, path );
}


/* -------------------------------------------------------------------------- */

/* Absolute prefix without globs should work. */
  bool
test_isMatchingAttrPathPrefix2( nix::EvalState & state )
{
  std::vector<attr_part>   prefix = { "packages" };
  std::vector<nix::Symbol> parsed;
  parsed.push_back( state.symbols.create( "packages" ) );
  parsed.push_back( state.symbols.create( "x86_64-linux" ) );
  parsed.push_back( state.symbols.create( "hello" ) );

  std::vector<nix::SymbolStr> path = state.symbols.resolve( parsed );

  return isMatchingAttrPathPrefix( prefix, path );
}


/* -------------------------------------------------------------------------- */

/* Empty prefix should always work. */
  bool
test_isMatchingAttrPathPrefix3( nix::EvalState & state )
{
  std::vector<attr_part>   prefix = {};
  std::vector<nix::Symbol> parsed;
  parsed.push_back( state.symbols.create( "packages" ) );
  parsed.push_back( state.symbols.create( "x86_64-linux" ) );
  parsed.push_back( state.symbols.create( "hello" ) );

  std::vector<nix::SymbolStr> path = state.symbols.resolve( parsed );

  return isMatchingAttrPathPrefix( prefix, path );
}


/* -------------------------------------------------------------------------- */

/* Assert that relative prefixes work. */
  bool
test_isMatchingAttrPathPrefix4( nix::EvalState & state )
{
  std::vector<attr_part>   prefix = { "python3", "pkgs" };
  std::vector<nix::Symbol> parsed;
  parsed.push_back( state.symbols.create( "packages" ) );
  parsed.push_back( state.symbols.create( "x86_64-linux" ) );
  parsed.push_back( state.symbols.create( "python3" ) );
  parsed.push_back( state.symbols.create( "pkgs" ) );
  parsed.push_back( state.symbols.create( "pip" ) );

  std::vector<nix::SymbolStr> path = state.symbols.resolve( parsed );

  return isMatchingAttrPathPrefix( prefix, path );
}


/* -------------------------------------------------------------------------- */

/* Assert that relative prefixes for an exact match work. */
  bool
test_isMatchingAttrPathPrefix5( nix::EvalState & state )
{
  std::vector<attr_part>   prefix = { "python3", "pkgs", "pip" };
  std::vector<nix::Symbol> parsed;
  parsed.push_back( state.symbols.create( "packages" ) );
  parsed.push_back( state.symbols.create( "x86_64-linux" ) );
  parsed.push_back( state.symbols.create( "python3" ) );
  parsed.push_back( state.symbols.create( "pkgs" ) );
  parsed.push_back( state.symbols.create( "pip" ) );

  std::vector<nix::SymbolStr> path = state.symbols.resolve( parsed );

  return isMatchingAttrPathPrefix( prefix, path );
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


#define RUN_TEST_WITH( _STATE, _NAME )                                 \
  try                                                                  \
    {                                                                  \
      if ( ! test_ ## _NAME ( _STATE ) )                               \
        {                                                              \
          ec = EXIT_FAILURE;                                           \
          std::cerr << "  fail: " # _NAME << std::endl;                \
        }                                                              \
    }                                                                  \
  catch( std::exception & e )                                          \
    {                                                                  \
      ec = EXIT_FAILURE;                                               \
      std::cerr << "  ERROR: " # _NAME ": " << e.what() << std::endl;  \
    }                                                                  \
  /* Clear state's symbol table. */                                    \
  state.symbols = nix::SymbolTable();


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

  nix::initNix();
  nix::initGC();
  nix::evalSettings.pureEval = false;
  nix::EvalState state( {}, nix::openStore() );

  RUN_TEST_WITH( state, isMatchingAttrPathPrefix1 );
  RUN_TEST_WITH( state, isMatchingAttrPathPrefix2 );
  RUN_TEST_WITH( state, isMatchingAttrPathPrefix3 );
  RUN_TEST_WITH( state, isMatchingAttrPathPrefix4 );
  RUN_TEST_WITH( state, isMatchingAttrPathPrefix5 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
