/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <nix/command.hh>
#include <nix/installables.hh>
#include <vector>


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/**
 * A command that operates on a list of "installables", which can be
 * store paths, attribute paths, Nix expressions, etc.
 */
struct FloxInstallablesCommand : nix::InstallablesCommand {
  /* Overrides for `RawInstallablesCommand' routines. */
  void run( nix::ref<nix::Store>        store
          , std::vector<std::string> && rawInstallables
          );
  void applyDefaultInstallables( std::vector<std::string> & rawInstallables );
  /* Overrides for `InstallablesCommand' routines. */
  void run( nix::ref<nix::Store>    store
          , nix::Installables    && installables
          );
};

/** A command that operates on exactly one "installable" */
struct FloxInstallableCommand : nix::InstallableCommand {
  /* Overrides for `SourceExprCommand' routines. */
  nix::Installables parseInstallables( nix::ref<nix::Store> store
                                     , std::vector<std::string> ss
                                     );
  nix::ref<nix::Installable> parseInstallable(
          nix::ref<nix::Store>   store
  , const std::string          & installable
  );
  nix::Strings getDefaultFlakeAttrPaths();
  nix::Strings getDefaultFlakeAttrPathPrefixes();
  void         completeInstallable( std::string_view prefix );
};


/* -------------------------------------------------------------------------- */

  void completeFloxRef( nix::ref<nix::Store> store, std::string_view prefix );

  void completeFlakeRefWithFragment(
          nix::ref<nix::EvalState>   evalState
  ,       nix::flake::LockFlags      lockFlags
  ,       nix::Strings               attrPathPrefixes
  , const nix::Strings             & defaultFlakeAttrPaths
  ,       std::string_view           prefix
  );


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
