/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <list>
#include <optional>
#include <nlohmann/json.hpp>
#include <nix/fetchers.hh>
#include <unordered_map>
#include <unordered_set>
#include <nix/eval-cache.hh>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

struct Descriptor {
  std::optional<std::list<std::string>> path;
  std::optional<std::string>            name;
  std::optional<std::string>            version;
  std::optional<std::string>            semver;

  bool                       searchCatalogs;
  std::optional<std::string> catalogId;
  std::optional<std::string> catalogStability;

  bool                       searchFlakes;
  std::optional<std::string> flakeId;

  Descriptor( const std::string_view   desc );
  Descriptor( const nlohmann::json   & desc );

  nlohmann::json toJSON()   const;
  std::string    toString() const;
};


void from_json( const nlohmann::json & j,       Descriptor & p );
void to_json(         nlohmann::json & j, const Descriptor & p );


/* -------------------------------------------------------------------------- */

class DescriptorFunctor {
  private:
    Descriptor * descriptor;

  public:
    bool shouldRecur( nix::eval_cache::AttrCursor & pos );
    bool packagePredicate( nix::eval_cache::AttrCursor & pos );
};


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
