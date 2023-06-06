/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <string>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/fetchers.hh>
#include <bits/stdc++.h>
#include "resolve.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

/* This initializer is incredibly annoying... */
Resolved::Resolved( std::string_view uri )
  : input(
      std::get<0>( nix::parseFlakeRefWithFragment( std::string( this->uri ) ) )
    )
{
  this->uri = uri;

  auto [ref, frag] = nix::parseFlakeRefWithFragment( this->uri );

  // TODO: Move to helper
  this->path.clear();
  std::string cur;
  auto i = frag.begin();
  while ( i != frag.end() )
    {
      if ( ( *i ) == '.' )
        {
          this->path.push_back( cur );
          cur.clear();
        }
      else if ( ( *i ) == '"' )
        {
          ++i;
          while ( true )
            {
              if ( i == frag.end() )
                {
                  throw ResolverException(
                      "missing closing quote in selection path " + frag
                  );
                }
              if ( ( *i ) == '"' ) { break; }
              cur.push_back( *i++ );
            }
        }
      else
        {
          cur.push_back( *i );
        }
      ++i;
    }
  if ( ! cur.empty() ) { this->path.push_back( cur ); }

  // TODO
  this->info = nlohmann::json::object();
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
