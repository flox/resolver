/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

using input_pair   =
  std::pair<std::string, std::shared_ptr<nix::flake::LockedFlake>>;


/* -------------------------------------------------------------------------- */

namespace predicates { struct PkgPred; };

struct Preferences {
  std::vector<std::string> inputs;

  std::unordered_map<std::string, std::vector<std::string>> stabilities;
  std::unordered_map<std::string, std::vector<std::string>> prefixes;

  bool semverPreferPreReleases = false;
  bool allowUnfree             = true;
  bool allowBroken             = false;

  std::optional<std::unordered_set<std::string>> allowedLicenses;

  Preferences() = default;
  Preferences( const nlohmann::json & j );

  nlohmann::json toJSON() const;

  flox::resolve::predicates::PkgPred pred_V2() const;

  int compareInputs(
        const std::string_view idA, const nix::FlakeRef & a
      , const std::string_view idB, const nix::FlakeRef & b
      ) const;

    inline int
  compareInputs( const input_pair & a, const input_pair & b ) const
  {
    return this->compareInputs( a.first
                              , a.second->flake.lockedRef
                              , b.first
                              , b.second->flake.lockedRef
                              );
  }

    std::function<bool( const input_pair &, const input_pair & )>
  inputLessThan() const
  {
    return [&]( const input_pair & a, const input_pair & b )
    {
      return this->compareInputs( a, b ) < 0;
    };
  }
};


void from_json( const nlohmann::json & j,       Preferences & p );
void to_json(         nlohmann::json & j, const Preferences & p );


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */



/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
