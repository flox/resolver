/* ========================================================================== *
 *
 * Lock inputs command.
 * This largely exists to serve as an example for using the `nix' arg parser
 * without registering commands or using `main' from `<nix>src/nix/main.cc'.
 *
 * -------------------------------------------------------------------------- */

#include <filesystem>
#include <fstream>
#include <string>
#include <nix/command.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/args.hh>
#include <nlohmann/json.hpp>
#include "resolve.hh"
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;

/* -------------------------------------------------------------------------- */

  static inline nlohmann::json
readOrParseJSON( const std::string & i )
{
  nlohmann::json j;
  if ( std::filesystem::exists( i ) )
    {
      j = nlohmann::json::parse( std::ifstream( i ) );
    }
  else
    {
      j = nlohmann::json::parse( i );
    }
  return j;
}


/* -------------------------------------------------------------------------- */

class CmdInputsLock : virtual nix::EvalCommand, public virtual nix::Args
{
  private:

    Inputs * _inputs = nullptr;

  public:

    CmdInputsLock()
    {
      this->expectArgs( nix::Args::ExpectedArg {
        .label     = "inputs"
      , .optional  = true
      , .handler   = {
          [&]( std::string istr )
          {
            this->_inputs = new Inputs(
              (nlohmann::json) readOrParseJSON( istr )
            );
          }
        }
      , .completer = { []( size_t, std::string_view ) {} }
      } );
    }

    ~CmdInputsLock() { delete this->_inputs; }


/* -------------------------------------------------------------------------- */

      std::string
    description() override
    {
      return "Lock a set of inputs.";
    }

    std::string doc() override { return this->description(); }  // TODO


/* -------------------------------------------------------------------------- */

      void
    run() override
    {
      nix::evalSettings.pureEval.setDefault( false );
      nix::ref<nix::EvalState> state = this->getEvalState();

      nlohmann::json rsl = nlohmann::json::object();
      if ( this->_inputs == nullptr )
        {
          this->_inputs = new Inputs( nlohmann::json {
            { "nixpkgs", "github:NixOS/nixpkgs" }
          } );
        }

      for ( auto & id : this->_inputs->getInputNames() )
        {
          nix::flake::LockedFlake lockedFlake =
            nix::flake::lockFlake( * state
                                 , this->_inputs->get( id )
                                 , floxFlakeLockFlags
                                 );
          rsl.emplace(
            id
          , nix::fetchers::attrsToJSON( lockedFlake.flake.lockedRef.toAttrs() )
          );
        }
      nix::logger->cout( "%s", rsl.dump() );
    }

    void run( nix::ref<nix::Store> ) override { this->run(); }

/* -------------------------------------------------------------------------- */

};  /* End class `CmdInputsLock' */


/* -------------------------------------------------------------------------- */

  void
mainWrapped( int argc, char * argv[] )
{
  /* Increase the default stack size. This aligns with `nix' new CLI usage. */
  nix::setStackSize( 64 * 1024 * 1024 );
  nix::initNix();
  nix::initGC();
  std::list<std::string> args;
  CmdInputsLock cmd;
  for ( int i = 1; i < argc; ++i )
    {
      if ( ( std::string_view( argv[i] ) == "-h" ) ||
           ( std::string_view( argv[i] ) == "--help" )
         )
        {
          nix::logger->cout(
            "lock-inputs: %s\nUSAGE:  lock-inputs [INPUTS]\n\nARGUMENTS\n"
            "  INPUTS  inline JSON string or path to JSON file.\n"
            "          JSON object must map aliases/ids to flake refs.\n\n"
            "Flake ref values may be either string URIs, or attrsets like\n"
            "those held in `flake.lock' and used by `builtins.fetchTree'."
          , cmd.description()
          );
          return;
        }
      args.push_back( argv[i] );
    }
  cmd.parseCmdline( args );
  cmd.run();
}


/* -------------------------------------------------------------------------- */

  int
main( int argc, char * argv[] )
{
  return nix::handleExceptions( argv[0], [&]() { mainWrapped( argc, argv ); } );
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
