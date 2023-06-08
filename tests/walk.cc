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
  std::vector<attr_part>      prefix = { "packages", nullptr };
  std::vector<nix::SymbolStr> path   = coerceSymbolStrs( state, {
    "packages", "x86_64-linux", "hello"
  } );
  return isMatchingAttrPathPrefix( prefix, path );
}


/* -------------------------------------------------------------------------- */

/* Absolute prefix without globs should work. */
  bool
test_isMatchingAttrPathPrefix2( nix::EvalState & state )
{
  std::vector<attr_part>      prefix = { "packages" };
  std::vector<nix::SymbolStr> path   = coerceSymbolStrs( state, {
    "packages", "x86_64-linux", "hello"
  } );
  return isMatchingAttrPathPrefix( prefix, path );
}


/* -------------------------------------------------------------------------- */

/* Empty prefix should always work. */
  bool
test_isMatchingAttrPathPrefix3( nix::EvalState & state )
{
  std::vector<nix::SymbolStr> path = coerceSymbolStrs( state, {
    "packages", "x86_64-linux", "hello"
  } );
  return isMatchingAttrPathPrefix( {}, path );
}


/* -------------------------------------------------------------------------- */

/* Assert that relative prefixes work. */
  bool
test_isMatchingAttrPathPrefix4( nix::EvalState & state )
{
  std::vector<attr_part>      prefix = { "python3", "pkgs" };
  std::vector<nix::SymbolStr> path   = coerceSymbolStrs( state, {
    "packages", "x86_64-linux", "python3", "pkgs", "pip"
  } );
  return isMatchingAttrPathPrefix( prefix, path );
}


/* -------------------------------------------------------------------------- */

/* Assert that relative prefixes for an exact match work. */
  bool
test_isMatchingAttrPathPrefix5( nix::EvalState & state )
{
  std::vector<attr_part>      prefix = { "python3", "pkgs", "pip" };
  std::vector<nix::SymbolStr> path   = coerceSymbolStrs( state, {
    "packages", "x86_64-linux", "python3", "pkgs", "pip"
  } );
  return isMatchingAttrPathPrefix( prefix, path );
}


/* -------------------------------------------------------------------------- */

/* Absolute ppath with globs should work. */
  bool
test_isMatchingAttrPath1( nix::EvalState & state )
{
  std::vector<attr_part>      prefix = { "packages", nullptr, "hello" };
  std::vector<nix::SymbolStr> path   = coerceSymbolStrs( state, {
    "packages", "x86_64-linux", "hello"
  } );
  return isMatchingAttrPath( prefix, path );
}


/* -------------------------------------------------------------------------- */

/* Absolute path without globs should work. */
  bool
test_isMatchingAttrPath2( nix::EvalState & state )
{
  std::vector<attr_part>      prefix = { "packages", "x86_64-linux", "hello" };
  std::vector<nix::SymbolStr> path   = coerceSymbolStrs( state, {
    "packages", "x86_64-linux", "hello"
  } );
  return isMatchingAttrPath( prefix, path );
}


/* -------------------------------------------------------------------------- */

/* Empty should work. */
  bool
test_isMatchingAttrPath3( nix::EvalState & state )
{
  return isMatchingAttrPath( {}, {} );
}


/* -------------------------------------------------------------------------- */

/* Assert that relative paths work. */
  bool
test_isMatchingAttrPath4( nix::EvalState & state )
{
  std::vector<attr_part>      prefix = { "python3", "pkgs", "pip" };
  std::vector<nix::SymbolStr> path   = coerceSymbolStrs( state, {
    "packages", "x86_64-linux", "python3", "pkgs", "pip"
  } );
  return isMatchingAttrPath( prefix, path );
}


/* -------------------------------------------------------------------------- */

  bool
test_shouldRecur1( nix::EvalState & state )
{
  std::string ref = "github:NixOS/nixpkgs/"
                    "e8039594435c68eb4f780f3e9bf3972a7399c4b1";

  nix::ref<nix::eval_cache::EvalCache>  cache = coerceEvalCache( state, ref );
  nix::ref<nix::eval_cache::AttrCursor> root  = cache->getRoot();

  Preferences       prefs;
  Descriptor        desc;
  DescriptorFunctor funk( state, prefs, desc );

  bool rsl = funk.shouldRecur( * root, {} );

  nix::ref<nix::eval_cache::AttrCursor> cur = root->getAttr( "legacyPackages" );
  std::vector<nix::Symbol>              path;
  path.push_back( state.symbols.create( "legacyPackages" ) );

  rsl &= funk.shouldRecur( * cur, path );

  cur = cur->getAttr( "x86_64-linux" );
  path.push_back( state.symbols.create( "x86_64-linux" ) );
  rsl &= funk.shouldRecur( * cur, path );

  cur = cur->getAttr( "hello" );
  path.push_back( state.symbols.create( "hello" ) );
  rsl &= ! funk.shouldRecur( * cur, path );

  return rsl;
}


/* -------------------------------------------------------------------------- */

  bool
test_nameVersionAt1( nix::EvalState & state )
{
  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( state, nixpkgsRef );
  nix::ref<nix::eval_cache::AttrCursor> root = cache->getRoot();

  Preferences       prefs;
  Descriptor        desc( (nlohmann::json) { { "name", "hello" } } );
  DescriptorFunctor funk( state, prefs, desc );

  nix::ref<nix::eval_cache::AttrCursor> cur =
    root->getAttr( "legacyPackages" )
        ->getAttr( "x86_64-linux" )
        ->getAttr( "hello" );

  PkgNameVersion pnv = nameVersionAt( "hello", * cur );

  return ( pnv.attrName      == "hello" ) &&
         ( pnv.name          == "hello-2.12.1" ) &&
         ( pnv.parsedName    == "hello" ) &&
         ( pnv.parsedVersion == "2.12.1" ) &&
         pnv.pname.has_value() &&
         ( pnv.pname.value() == "hello" ) &&
         pnv.version.has_value() &&
         ( pnv.version.value() == "2.12.1" );
}


/* -------------------------------------------------------------------------- */

  bool
test_packagePredicate1( nix::EvalState & state )
{
  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( state, nixpkgsRef );
  nix::ref<nix::eval_cache::AttrCursor> root = cache->getRoot();

  Preferences       prefs;
  Descriptor        desc( (nlohmann::json) { { "name", "hello" } } );
  DescriptorFunctor funk( state, prefs, desc );

  nix::ref<nix::eval_cache::AttrCursor> cur =
    root->getAttr( "legacyPackages" )
        ->getAttr( "x86_64-linux" )
        ->getAttr( "hello" );

  std::vector<nix::Symbol> path = coerceSymbols( state, {
    "legacyPackages", "x86_64-linux", "hello"
  } );

  return funk.packagePredicate( * cur, path );
}


/* -------------------------------------------------------------------------- */

  bool
test_packagePredicate2( nix::EvalState & state )
{
  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( state, nixpkgsRef );
  nix::ref<nix::eval_cache::AttrCursor> root = cache->getRoot();

  Preferences prefs;
  Descriptor  desc( (nlohmann::json) {
    { "name",    "hello" }
  , { "version", "2.12.1" }
  } );
  DescriptorFunctor funk( state, prefs, desc );

  nix::ref<nix::eval_cache::AttrCursor> cur =
    root->getAttr( "legacyPackages" )
        ->getAttr( "x86_64-linux" )
        ->getAttr( "hello" );

  std::vector<nix::Symbol> path = coerceSymbols( state, {
    "legacyPackages", "x86_64-linux", "hello"
  } );

  return funk.packagePredicate( * cur, path );
}


/* -------------------------------------------------------------------------- */

  bool
test_packagePredicate3( nix::EvalState & state )
{
  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( state, nixpkgsRef );
  nix::ref<nix::eval_cache::AttrCursor> root = cache->getRoot();

  Preferences       prefs;
  Descriptor        desc( (nlohmann::json) { { "name", "hello-2.12.1" } } );
  DescriptorFunctor funk( state, prefs, desc );

  nix::ref<nix::eval_cache::AttrCursor> cur =
    root->getAttr( "legacyPackages" )
        ->getAttr( "x86_64-linux" )
        ->getAttr( "hello" );

  std::vector<nix::Symbol> path = coerceSymbols( state, {
    "legacyPackages", "x86_64-linux", "hello"
  } );

  return funk.packagePredicate( * cur, path );
}


/* -------------------------------------------------------------------------- */

/* Assert that wrong name fails */
  bool
test_packagePredicate4( nix::EvalState & state )
{
  nix::ref<nix::eval_cache::EvalCache> cache =
    coerceEvalCache( state, nixpkgsRef );
  nix::ref<nix::eval_cache::AttrCursor> root = cache->getRoot();

  Preferences       prefs;
  Descriptor        desc( (nlohmann::json) { { "name", "helloooo" } } );
  DescriptorFunctor funk( state, prefs, desc );

  nix::ref<nix::eval_cache::AttrCursor> cur =
    root->getAttr( "legacyPackages" )
        ->getAttr( "x86_64-linux" )
        ->getAttr( "hello" );

  std::vector<nix::Symbol> path = coerceSymbols( state, {
    "legacyPackages", "x86_64-linux", "hello"
  } );

  return ! funk.packagePredicate( * cur, path );
}


/* -------------------------------------------------------------------------- */

/* Assert that wrong name fails */
  bool
test_walk( nix::EvalState & state )
{
  FloxFlakeRef                         ref   = coerceFlakeRef( nixpkgsRef );
  nix::ref<nix::eval_cache::EvalCache> cache = coerceEvalCache( state, ref );

  Preferences       prefs;
  Descriptor        desc( (nlohmann::json) { { "name", "helloooo" } } );
  DescriptorFunctor funk( state, prefs, desc );

  nix::ref<nix::eval_cache::AttrCursor> root = cache->getRoot();
  std::vector<nix::Symbol>              path;

  std::function<void(
    nix::eval_cache::AttrCursor    & cur
  , const std::vector<nix::Symbol> & attrPath
  )> visit;

  visit = [&](
    nix::eval_cache::AttrCursor    & cur
  , const std::vector<nix::Symbol> & attrPath
  ) -> void
  {
    std::vector<nix::SymbolStr> attrPathS = state.symbols.resolve( attrPath );

    #if 0
    std::cerr << "Visiting: ";
    for ( size_t i = 0; i < attrPathS.size(); ++i )
      {
        std::cerr << attrPathS[i];
        if ( i != ( attrPathS.size() - 1 ) )
          {
            std::cerr << '.';
          }
      }
    std::cerr << std::endl;
    #endif

    if ( funk.shouldRecur( cur, attrPath ) )
      {
        for ( const auto & attr : cur.getAttrs() )
          {
            try
              {
                std::vector<nix::Symbol> attrPath2( attrPath );
                attrPath2.push_back( attr );
                nix::ref<nix::eval_cache::AttrCursor> child =
                  cur.getAttr( state.symbols[attr] );
                visit( * child, attrPath2 );
              }
            catch ( nix::EvalError & e )
              {
                if ( ! ( ( attrPathS[0] == "legacyPackages" ) &&
                         ( 0 < attrPath.size() ) )
                   )
                  {
                    throw;
                  }
              }
          }
      }
    else if ( cur.isDerivation() && funk.packagePredicate( cur, attrPath ) )
      {
        std::vector<attr_part> globPath;
        for ( size_t i = 0; i < attrPathS.size(); ++i )
          {
            if ( i == 1 ) { globPath.push_back( nullptr ); }
            else          { globPath.push_back( attrPathS[i] ); }
          }
        PkgNameVersion pnv =
          nameVersionAt( attrPathS[attrPath.size() - 1], cur );
        // TODO: Check if present and append `systems'.
        Resolved r(
          ref
        , globPath
        , (nlohmann::json) {
            { "name",    pnv.pname.value_or( pnv.parsedName ) }
          , { "version", pnv.version.value_or( pnv.parsedVersion ) }
          , { "systems", { attrPathS[1] } }
          }
        );
        funk.results.push_back( r );
      }
  };  /* End `visit' */

  try
    {
      std::shared_ptr<nix::eval_cache::AttrCursor> st =
        root->maybeGetAttr( "packages" );
      if ( st != nullptr )
        {
          visit( * st, { state.symbols.create( "packages" ) } );
        }
    }
  catch( std::exception & e ) { std::cerr << e.what() << std::endl; }
  catch( ... ) {}

  try
    {
      std::shared_ptr<nix::eval_cache::AttrCursor> st =
        root->maybeGetAttr( "legacyPackages" );
      if ( st != nullptr )
        {
          visit( * st, { state.symbols.create( "legacyPackages" ) } );
        }
    }
  catch( std::exception & e ) { std::cerr << e.what() << std::endl; }
  catch( ... ) {}

  try
    {
      std::shared_ptr<nix::eval_cache::AttrCursor> st =
        root->maybeGetAttr( "catalog" );
      if ( st != nullptr )
        {
          visit( * st, { state.symbols.create( "catalog" ) } );
        }
    }
  catch( std::exception & e ) { std::cerr << e.what() << std::endl; }
  catch( ... ) {}

  return true;
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
  RUN_TEST_WITH_STATE( state, nameVersionAt1 );
  RUN_TEST_WITH_STATE( state, packagePredicate1 );
  RUN_TEST_WITH_STATE( state, packagePredicate2 );
  RUN_TEST_WITH_STATE( state, packagePredicate3 );
  RUN_TEST_WITH_STATE( state, packagePredicate4 );

  RUN_TEST_WITH_STATE( state, walk );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
