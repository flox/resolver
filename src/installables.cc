/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox-installables.hh"
#include <string>
#include <vector>


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

  void
FloxInstallablesCommand::run( nix::ref<nix::Store>        store
                            , std::vector<std::string> && rawInstallables
                            )
{

}


/* -------------------------------------------------------------------------- */

  void
FloxInstallablesCommand::run( nix::ref<nix::Store>    store
                            , nix::Installables    && installables
                            )
{

}


/* -------------------------------------------------------------------------- */


  void
FloxInstallablesCommand::applyDefaultInstallables(
  std::vector<std::string> & rawInstallables
)
{

}


/* -------------------------------------------------------------------------- */

  nix::Installables
FloxInstallableCommand::parseInstallables( nix::ref<nix::Store>     store
                                         , std::vector<std::string> ss
                                         )
{
  return {};
}


//  nix::ref<nix::Installable>
//FloxInstallableCommand::parseInstallable(
//  nix::ref<nix::Store>   store
//, const std::string          & installable
//)
//{
//  nix::Installable i;
//  return i;
//}


  nix::Strings
FloxInstallableCommand::getDefaultFlakeAttrPaths()
{
  return {};
}


  nix::Strings
FloxInstallableCommand::getDefaultFlakeAttrPathPrefixes()
{
  return {};
}


  void
FloxInstallableCommand::completeInstallable( std::string_view prefix )
{

}


/* -------------------------------------------------------------------------- */

  void
completeFloxRef( nix::ref<nix::Store> store, std::string_view prefix )
{

}


  void
completeFloxRefWithFragment(
        nix::ref<nix::EvalState>   evalState
,       nix::flake::LockFlags      lockFlags
,       nix::Strings               attrPathPrefixes
, const nix::Strings             & defaultFlakeAttrPaths
,       std::string_view           prefix
)
{

}


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
