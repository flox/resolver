/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/fetchers.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include <nix/command.hh>
#include <string>
#include <nlohmann/json.hpp>
#include "resolve.hh"
#include "flox/util.hh"
#include <functional>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  std::list<Resolved>
resolve_V2( ResolverState & rs, const Descriptor & desc, bool one )
{
  /* See if we can take shortcuts with this descriptor. */
  if ( desc.inputId.has_value() )
    {
      return rs.resolveInInput( desc.inputId.value(), desc );
    }
  std::list<Resolved> results;
  for ( const std::string_view & id : rs.getInputNames() )
    {
      results.splice( results.end(), rs.resolveInInput( id, desc ) );
      if ( one && ( ! results.empty() ) )
        {
          results.erase( ++results.begin(), results.end() );
          return results;
        }
    }
  return results;
}


/* -------------------------------------------------------------------------- */

  std::optional<Resolved>
resolveOne_V2( ResolverState & rs, const Descriptor & desc )
{
  std::list<Resolved> resolved = resolve_V2( rs, desc, true );
  if ( resolved.empty() ) { return std::nullopt; }
  return resolved.front();
}


/* -------------------------------------------------------------------------- */

  void
to_json( nlohmann::json & j, const Resolved & r )
{
  j = r.toJSON();
}

  void
from_json( const nlohmann::json & j, Resolved & r )
{
  r = Resolved( j );
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
