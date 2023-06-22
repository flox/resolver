/* ========================================================================== *
 *
 * DrvDb CLI frontend.
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
#include "flox/drv-cache.hh"


/* -------------------------------------------------------------------------- */

using namespace flox::resolve;

/* -------------------------------------------------------------------------- */

class CmdGetProgress : public virtual nix::Args
{
  private:

    Inputs        * _inputs = nullptr;
    ResolverState * _rs     = nullptr;

  public:

    CmdGetProgress()
    {
      this->expectArgs( {
        .label     = "flake"
      , .optional  = true
      , .handler   = {
          [&]( std::string istr )
          {
            try
              {
                this->_inputs = new Inputs( (nlohmann::json) {
                  { "target", nlohmann::json::parse( istr ) }
                } );
              }
            catch( ... )
              {
                this->_inputs = new Inputs( (nlohmann::json) {
                  { "target", istr }
                } );
              }
          }
        }
      } );
    }

    ~CmdGetProgress()
    {
      if ( this->_rs     != nullptr ) { delete this->_rs;     }
      if ( this->_inputs != nullptr ) { delete this->_inputs; }
    }


/* -------------------------------------------------------------------------- */

      std::string
    description() override
    {
      return "Check derivation database status/progress for a flake.";
    }

    std::string doc() override { return this->description(); }  // TODO


/* -------------------------------------------------------------------------- */

      void
    run()
    {
      if ( this->_rs == nullptr )
        {
          assert( this->_inputs != nullptr );
          Preferences prefs;
          this->_rs = new ResolverState( * this->_inputs, prefs );
          delete this->_inputs;
          this->_inputs = nullptr;
        }
      nix::ref<FloxFlake> flake = this->_rs->getInput( "target" ).value();
      std::string dbName = nix::getCacheDir() + "/flox/drv-cache-v0/"
        + flake->getLockedFlake()->getFingerprint().to_string( nix::Base16
                                                             , false
                                                             )
        + ".sqlite";
      if ( ! std::filesystem::exists( dbName) )
        {
          nix::logger->cout( "NO DB" );
          return;
        }
      DrvDb db( flake->getLockedFlake()->getFingerprint() );
      for ( auto & [subtree, subs] : db.getProgresses() )
        {
          for ( auto & [system, status] : std::move( subs ) )
            {
              nix::logger->cout( "%s.%s %s"
                               , subtree
                               , std::move( system )
                               , progressStatusToString( std::move( status ) )
                               );
            }
        }
    }

/* -------------------------------------------------------------------------- */

};  /* End class `CmdGetProgress' */


/* -------------------------------------------------------------------------- */

  void
mainWrapped( int argc, char * argv[] )
{
  /* Increase the default stack size. This aligns with `nix' new CLI usage. */
  nix::setStackSize( 64 * 1024 * 1024 );
  nix::initNix();
  nix::initGC();
  std::list<std::string> args;
  CmdGetProgress cmd;
  for ( size_t i = 1; i < argc; ++i )
    {
      if ( ( std::string_view( argv[i] ) == "-h" ) ||
           ( std::string_view( argv[i] ) == "--help" )
         )
        {
          nix::logger->cout(
            "db progress: %s\nUSAGE:  lock-inputs INPUTS\n\nARGUMENTS\n"
            "  INPUT  inline JSON object or string representing a flake ref\n\n"
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
main( int argc, char * argv[], char ** envp )
{
  return nix::handleExceptions( argv[0], [&]() { mainWrapped( argc, argv ); } );
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
