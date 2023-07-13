/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "test.hh"
#include "flox/cursor-iter.hh"


/* -------------------------------------------------------------------------- */

using namespace flox;
using namespace flox::resolve;
using namespace nlohmann::literals;

/* -------------------------------------------------------------------------- */

  bool
test_cursor_iterator1( ResolverState & rs, nix::ref<FloxFlake> flake )
{
  Cursor curr = flake->openCursor(
    (std::vector<std::string>) { "legacyPackages", "x86_64-linux" }
  );
  detail::cursor_iterator_closure( rs.getEvalState(), curr );
  return true;
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[], char ** envp )
{
  int ec = EXIT_SUCCESS;
# define RUN_TEST( ... )  _RUN_TEST( ec, __VA_ARGS__ )

  Inputs      inputs( (nlohmann::json) { { "nixpkgs", nixpkgsRef } } );
  Preferences prefs;
  ResolverState rs(
    inputs
  , prefs
  , (std::list<std::string>) { "x86_64-linux" }
  );

  nix::ref<FloxFlake> flake = rs.getInput( "nixpkgs" ).value();

  RUN_TEST( cursor_iterator1, rs, flake );

  return ec;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
