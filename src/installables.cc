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


  nix::ref<nix::Installable>
parseInstallable(       nix::ref<nix::Store>   store
                , const std::string          & installable
  )
{
  nix::Installable i;
  return i;
}


  nix::Strings
getDefaultFlakeAttrPaths()
{
  return {};
}


  nix::Strings
getDefaultFlakeAttrPathPrefixes()
{
  return {};
}


  void
completeInstallable( std::string_view prefix )
{

}


/* -------------------------------------------------------------------------- */

}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
