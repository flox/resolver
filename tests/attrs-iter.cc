/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "test.hh"
#include "flox/types.hh"
#include "flox/resolver-state.hh"
#include "flox/attrs-iter.hh"


/* -------------------------------------------------------------------------- */

using namespace flox;
using namespace flox::resolve;
using namespace flox::util;

/* -------------------------------------------------------------------------- */

  bool
test_iter1(
  ResolverState                            & rs
, std::shared_ptr<nix::flake::LockedFlake>   flake
)
{
  std::list<std::string> path = {};
  AttrSetIterClosure cl( rs.getEvalState(), flake, path );
  size_t c = 0;
  for ( auto [path, cursor] : cl ) { ++c; }
  /**
   * checks
   * htmlDocs
   * legacyPackages
   * lib
   * nixosModules
   */
  return c == 5;
}

/* -------------------------------------------------------------------------- */

/* Test const refs */
  bool
test_iter2(
  ResolverState                            & rs
, std::shared_ptr<nix::flake::LockedFlake>   flake
)
{
  std::list<std::string> path = {};
  AttrSetIterClosure cl( rs.getEvalState(), flake, path );
  size_t c = 0;
  for ( const auto & [path, cursor] : cl ) { ++c; }
  return c == 5;
}

/* -------------------------------------------------------------------------- */

  int
main()
{
  int ec = EXIT_SUCCESS;

  Inputs      inputs( nlohmann::json { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs;
  ResolverState rs(
    inputs
  , prefs
  , std::list<std::string> { "x86_64-linux" }
  );

  std::shared_ptr<nix::flake::LockedFlake> flake  =
    rs.getInput( "nixpkgs" ).value()->getLockedFlake();

# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )
  RUN_TEST( iter1, rs, flake );
  RUN_TEST( iter2, rs, flake );
  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
