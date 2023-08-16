/* ========================================================================== *
 *
 * @file flox/resolver/util.hh
 *
 * @brief Miscellaneous helper functions.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */
/**
 * Predicate which indicates whether a `storePath' is "substitutable".
 * @param storePath an absolute path in the `/nix/store'.
 *        This should be an `outPath' and NOT a `drvPath' in most cases.
 * @return true iff `storePath' is cached in a remote `nix' store and can be
 *              copied without being "rebuilt" from scratch.
 */
bool isSubstitutable( std::string_view storePath );


/* -------------------------------------------------------------------------- */

}  /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
