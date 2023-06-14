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
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

static const std::string nixpkgsRef =
  "github:NixOS/nixpkgs/e8039594435c68eb4f780f3e9bf3972a7399c4b1";

static const std::string nixpkgsFloxRef =
  "github:flox/nixpkgs-flox/b106c2d78d4258d781488743178a6ec63dc555ba";


/* -------------------------------------------------------------------------- */

/* Absolute prefix with globs should work. */
  bool
test_isMatchingAttrPathPrefix1( nix::ref<nix::EvalState> state )
{
  std::vector<nix::SymbolStr> path   = coerceSymbolStrs( * state, {
    "packages", "x86_64-linux", "hello"
  } );
  return isMatchingAttrPathPrefix( AttrPathGlob( { "packages", nullptr } )
                                 , path
                                 );
}


/* -------------------------------------------------------------------------- */

/* Absolute prefix without globs should work. */
  bool
test_isMatchingAttrPathPrefix2( nix::ref<nix::EvalState> state )
{
  std::vector<nix::SymbolStr> path = coerceSymbolStrs( * state, {
    "packages", "x86_64-linux", "hello"
  } );
  return isMatchingAttrPathPrefix( AttrPathGlob( { "packages" } ), path );
}


/* -------------------------------------------------------------------------- */

/* Empty prefix should always work. */
  bool
test_isMatchingAttrPathPrefix3( nix::ref<nix::EvalState> state )
{
  std::vector<nix::SymbolStr> path = coerceSymbolStrs( * state, {
    "packages", "x86_64-linux", "hello"
  } );
  return isMatchingAttrPathPrefix( AttrPathGlob(), path );
}


/* -------------------------------------------------------------------------- */

/* Assert that relative prefixes work. */
  bool
test_isMatchingAttrPathPrefix4( nix::ref<nix::EvalState> state )
{
  std::vector<nix::SymbolStr> path = coerceSymbolStrs( * state, {
    "packages", "x86_64-linux", "python3", "pkgs", "pip"
  } );
  return isMatchingAttrPathPrefix( AttrPathGlob( { "python3", "pkgs" } )
                                 , path
                                 );
}


/* -------------------------------------------------------------------------- */

/* Assert that relative prefixes for an exact match work. */
  bool
test_isMatchingAttrPathPrefix5( nix::ref<nix::EvalState> state )
{
  std::vector<attr_part>      prefix = { "python3", "pkgs", "pip" };
  std::vector<nix::SymbolStr> path   = coerceSymbolStrs( * state, {
    "packages", "x86_64-linux", "python3", "pkgs", "pip"
  } );
  return isMatchingAttrPathPrefix( AttrPathGlob( prefix ), path );
}


/* -------------------------------------------------------------------------- */

/* Absolute ppath with globs should work. */
  bool
test_isMatchingAttrPath1( nix::ref<nix::EvalState> state )
{
  std::vector<attr_part>      prefix = { "packages", nullptr, "hello" };
  std::vector<nix::SymbolStr> path   = coerceSymbolStrs( * state, {
    "packages", "x86_64-linux", "hello"
  } );
  return isMatchingAttrPath( AttrPathGlob( prefix ), path );
}


/* -------------------------------------------------------------------------- */

/* Absolute path without globs should work. */
  bool
test_isMatchingAttrPath2( nix::ref<nix::EvalState> state )
{
  std::vector<attr_part>      prefix = { "packages", "x86_64-linux", "hello" };
  std::vector<nix::SymbolStr> path   = coerceSymbolStrs( * state, {
    "packages", "x86_64-linux", "hello"
  } );
  return isMatchingAttrPath( AttrPathGlob( prefix ), path );
}


/* -------------------------------------------------------------------------- */

/* Empty should work. */
  bool
test_isMatchingAttrPath3( nix::ref<nix::EvalState> state )
{
  return isMatchingAttrPath( AttrPathGlob(), {} );
}


/* -------------------------------------------------------------------------- */

/* Assert that relative paths work. */
  bool
test_isMatchingAttrPath4( nix::ref<nix::EvalState> state )
{
  std::vector<nix::SymbolStr> path = coerceSymbolStrs( * state, {
    "packages", "x86_64-linux", "python3", "pkgs", "pip"
  } );
  return isMatchingAttrPath( AttrPathGlob( { "python3", "pkgs", "pip" } )
                           , path
                           );
}


/* -------------------------------------------------------------------------- */

  bool
test_shouldRecur1( nix::ref<nix::EvalState> state )
{
  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( * state, nixpkgsRef );
  Cursor root = cache->getRoot();

  Preferences       prefs;
  Descriptor        desc;
  DescriptorFunctor funk( state, prefs, desc );

  bool rsl = funk.shouldRecur( root, {} );

  Cursor cur = root->getAttr( "legacyPackages" );
  std::vector<nix::Symbol>              path;
  path.push_back( state->symbols.create( "legacyPackages" ) );

  rsl &= funk.shouldRecur( cur, path );

  cur = cur->getAttr( "x86_64-linux" );
  path.push_back( state->symbols.create( "x86_64-linux" ) );
  rsl &= funk.shouldRecur( cur, path );

  cur = cur->getAttr( "hello" );
  path.push_back( state->symbols.create( "hello" ) );
  rsl &= ! funk.shouldRecur( cur, path );

  return rsl;
}


/* -------------------------------------------------------------------------- */

  bool
test_shouldRecur2( nix::ref<nix::EvalState> state )
{
  /* Push current verbosity */
  nix::Verbosity oldV = nix::verbosity;
  nix::verbosity      = nix::lvlError;

  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( * state, nixpkgsFloxRef );
  /* Pop verbosity */
  nix::verbosity = oldV;

  Cursor root = cache->getRoot();

  Preferences       prefs;
  Descriptor        desc;
  DescriptorFunctor funk( state, prefs, desc );

  bool rsl = funk.shouldRecur( root, {} );

  Cursor cur = root->getAttr( "catalog" );
  std::vector<nix::Symbol> path;
  path.push_back( state->symbols.create( "catalog" ) );

  rsl &= funk.shouldRecur( cur, path );

  cur = cur->getAttr( "x86_64-linux" );
  path.push_back( state->symbols.create( "x86_64-linux" ) );
  rsl &= funk.shouldRecur( cur, path );

  cur = cur->getAttr( "stable" );
  path.push_back( state->symbols.create( "stable" ) );
  rsl &= funk.shouldRecur( cur, path );

  cur = cur->getAttr( "hello" );
  path.push_back( state->symbols.create( "hello" ) );
  rsl &= funk.shouldRecur( cur, path );

  cur = cur->getAttr( "latest" );
  path.push_back( state->symbols.create( "latest" ) );
  rsl &= ! funk.shouldRecur( cur, path );

  return rsl;
}


/* -------------------------------------------------------------------------- */

  bool
test_nameVersionAt1( nix::ref<nix::EvalState> state )
{
  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( * state, nixpkgsRef );
  Cursor root = cache->getRoot();

  Preferences       prefs;
  Descriptor        desc( (nlohmann::json) { { "name", "hello" } } );
  DescriptorFunctor funk( state, prefs, desc );

  Cursor cur =
    root->getAttr( "legacyPackages" )
        ->getAttr( "x86_64-linux" )
        ->getAttr( "hello" );

  PkgNameVersion pnv = nameVersionAt( cur );

  return ( pnv.name == "hello-2.12.1" ) &&
         pnv.pname.has_value() && ( pnv.pname.value() == "hello" ) &&
         pnv.version.has_value() && ( pnv.version.value() == "2.12.1" );
}


/* -------------------------------------------------------------------------- */

  bool
test_packagePredicate1( nix::ref<nix::EvalState> state )
{
  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( * state, nixpkgsRef );
  Cursor root = cache->getRoot();

  Preferences       prefs;
  Descriptor        desc( (nlohmann::json) { { "name", "hello" } } );
  DescriptorFunctor funk( state, prefs, desc );

  Cursor cur =
    root->getAttr( "legacyPackages" )
        ->getAttr( "x86_64-linux" )
        ->getAttr( "hello" );

  std::vector<nix::Symbol> path = coerceSymbols( * state, {
    "legacyPackages", "x86_64-linux", "hello"
  } );

  return funk.packagePredicate( cur, path );
}


/* -------------------------------------------------------------------------- */

  bool
test_packagePredicate2( nix::ref<nix::EvalState> state )
{
  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( * state, nixpkgsRef );
  Cursor root = cache->getRoot();

  Preferences prefs;
  Descriptor  desc( (nlohmann::json) {
    { "name",    "hello" }
  , { "version", "2.12.1" }
  } );
  DescriptorFunctor funk( state, prefs, desc );

  Cursor cur =
    root->getAttr( "legacyPackages" )
        ->getAttr( "x86_64-linux" )
        ->getAttr( "hello" );

  std::vector<nix::Symbol> path = coerceSymbols( * state, {
    "legacyPackages", "x86_64-linux", "hello"
  } );

  return funk.packagePredicate( cur, path );
}


/* -------------------------------------------------------------------------- */

  bool
test_packagePredicate3( nix::ref<nix::EvalState> state )
{
  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( * state, nixpkgsRef );
  Cursor root = cache->getRoot();

  Preferences       prefs;
  Descriptor        desc( (nlohmann::json) { { "name", "hello-2.12.1" } } );
  DescriptorFunctor funk( state, prefs, desc );

  Cursor cur =
    root->getAttr( "legacyPackages" )
        ->getAttr( "x86_64-linux" )
        ->getAttr( "hello" );

  std::vector<nix::Symbol> path = coerceSymbols( * state, {
    "legacyPackages", "x86_64-linux", "hello"
  } );

  return funk.packagePredicate( cur, path );
}


/* -------------------------------------------------------------------------- */

/* Assert that wrong name fails */
  bool
test_packagePredicate4( nix::ref<nix::EvalState> state )
{
  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( * state, nixpkgsRef );
  Cursor root = cache->getRoot();

  Preferences       prefs;
  Descriptor        desc( (nlohmann::json) { { "name", "helloooo" } } );
  DescriptorFunctor funk( state, prefs, desc );

  Cursor cur = root->getAttr( "legacyPackages" )
                    ->getAttr( "x86_64-linux" )
                    ->getAttr( "hello" );

  std::vector<nix::Symbol> path = coerceSymbols( * state, {
    "legacyPackages", "x86_64-linux", "hello"
  } );

  return ! funk.packagePredicate( cur, path );
}


/* -------------------------------------------------------------------------- */

/* Ensure predicates work on catalog packages. */
  bool
test_packagePredicate5( nix::ref<nix::EvalState> state )
{
  /* Push current verbosity */
  nix::Verbosity oldV = nix::verbosity;
  nix::verbosity      = nix::lvlError;

  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( * state, nixpkgsFloxRef );
  /* Pop verbosity */
  nix::verbosity = oldV;

  Cursor root = cache->getRoot();

  Preferences       prefs;
  Descriptor        desc( (nlohmann::json) { { "name", "hello" } } );
  DescriptorFunctor funk( state, prefs, desc );

  Cursor cur = root->getAttr( "catalog" )
                    ->getAttr( "x86_64-linux" )
                    ->getAttr( "stable" )
                    ->getAttr( "hello" )
                    ->getAttr( "latest" );

  std::vector<nix::Symbol> path = coerceSymbols( * state, {
    "catalog", "x86_64-linux", "stable", "hello", "latest"
  } );

  return funk.packagePredicate( cur, path );
}


/* -------------------------------------------------------------------------- */

/* We should just get a single GNU `hello' as a result.
 * This package has the same name/version on all systems. */
  bool
test_walk1( nix::ref<nix::EvalState> state )
{
  FloxFlakeRef                         ref   = coerceFlakeRef( nixpkgsRef );
  nix::ref<nix::eval_cache::EvalCache> cache = coerceEvalCache( * state, ref );

  Preferences       prefs;
  Descriptor        desc( (nlohmann::json) { { "name", "hello" } } );
  DescriptorFunctor funk( state, prefs, desc );

  Cursor                   root = cache->getRoot();
  std::vector<nix::Symbol> path;

  /* Traverse attrsets and collect satisfactory packages. */
  funk.visit( ref, root, {} );
  return funk.results.size() == 1;
}


/* -------------------------------------------------------------------------- */

/* Expect 1 entry for packages such as `legacyPackages.{{system}}.bintools'
 * which have different names and versions for different systems.
 * The difference in name/version should not effect number of results.
 */
  bool
test_walk2( nix::ref<nix::EvalState> state )
{
  FloxFlakeRef                         ref   = coerceFlakeRef( nixpkgsRef );
  nix::ref<nix::eval_cache::EvalCache> cache = coerceEvalCache( * state, ref );

  Preferences prefs;
  Descriptor  desc( (nlohmann::json) {
    { "path", { "legacyPackages", nullptr, "bintools" } }
  } );
  DescriptorFunctor funk( state, prefs, desc );

  Cursor                   root = cache->getRoot();
  std::vector<nix::Symbol> path;

  /* Traverse attrsets and collect satisfactory packages. */
  funk.visit( ref, root, {} );
  return funk.results.size() == 1;
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


#define RUN_TEST_WITH_STATE( _STATE, _NAME )                           \
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
    }


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;

  nix::initNix();
  nix::initGC();
  nix::evalSettings.pureEval = true;  /* Our reference is locked so we can. */
  nix::ref<nix::EvalState> state( new nix::EvalState( {}, nix::openStore() ) );

  RUN_TEST_WITH_STATE( state, isMatchingAttrPathPrefix1 );
  RUN_TEST_WITH_STATE( state, isMatchingAttrPathPrefix2 );
  RUN_TEST_WITH_STATE( state, isMatchingAttrPathPrefix3 );
  RUN_TEST_WITH_STATE( state, isMatchingAttrPathPrefix4 );
  RUN_TEST_WITH_STATE( state, isMatchingAttrPathPrefix5 );
  RUN_TEST_WITH_STATE( state, isMatchingAttrPath1 );
  RUN_TEST_WITH_STATE( state, isMatchingAttrPath2 );
  RUN_TEST_WITH_STATE( state, isMatchingAttrPath3 );
  RUN_TEST_WITH_STATE( state, isMatchingAttrPath4 );

  RUN_TEST_WITH_STATE( state, shouldRecur1 );
  RUN_TEST_WITH_STATE( state, shouldRecur2 );
  RUN_TEST_WITH_STATE( state, nameVersionAt1 );
  RUN_TEST_WITH_STATE( state, packagePredicate1 );
  RUN_TEST_WITH_STATE( state, packagePredicate2 );
  RUN_TEST_WITH_STATE( state, packagePredicate3 );
  RUN_TEST_WITH_STATE( state, packagePredicate4 );
  RUN_TEST_WITH_STATE( state, packagePredicate5 );

  RUN_TEST_WITH_STATE( state, walk1 );
  RUN_TEST_WITH_STATE( state, walk2 );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
