/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "resolve.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

Preferences::Preferences( const nlohmann::json & j )
{
  for ( auto & [key, value] : j.items() )
    {
      if ( key == "inputs" )
        {
          this->inputs = value;
        }
      else if ( key == "prefixes" )
        {
          for ( auto & [input, order] : value.items() )
            {
              this->prefixes.emplace( input, order );
            }
        }
      else if ( key == "stabilities" )
        {
          for ( auto & [input, order] : value.items() )
            {
              this->stabilities.emplace( input, order );
            }
        }
      else if ( key == "semver" )
        {
          for ( auto & [skey, svalue] : value.items() )
            {
              if ( skey == "preferPreReleases" )
                {
                  this->semverPreferPreReleases = svalue;
                }
            }
        }
      else if ( key == "allow" )
        {
          for ( auto & [akey, avalue] : value.items() )
            {
              if ( akey == "licenses" )
                {
                  this->allowedLicenses = avalue;
                }
              else if ( akey == "unfree" )
                {
                  this->allowUnfree = avalue;
                }
              else if ( akey == "broken" )
                {
                  this->allowBroken = avalue;
                }
            }
        }
    }
}


/* -------------------------------------------------------------------------- */

  nlohmann::json
Preferences::toJSON() const
{
  nlohmann::json j;
  if ( ! this->inputs.empty() )
    {
      j.emplace( "inputs", this->inputs );
    }

  if ( ! this->stabilities.empty() )
    {
      nlohmann::json stabilities;
      for ( auto & [input, order] : this->stabilities )
        {
          stabilities.emplace( input, order );
        }
      j.emplace( "stabilities", stabilities );
    }

  if ( ! this->prefixes.empty() )
    {
      nlohmann::json prefixes;
      for ( auto & [input, order] : this->prefixes )
        {
          prefixes.emplace( input, order );
        }
      j.emplace( "prefixes", prefixes );
    }

  nlohmann::json semver = {
    { "preferPreReleases", this->semverPreferPreReleases }
  };
  j.emplace( "semver", semver );

  nlohmann::json allow = {
    { "broken", this->allowBroken }
  , { "unfree", this->allowUnfree }
  };

  if ( this->allowedLicenses.has_value() )
    {
      allow.emplace( "licenses", this->allowedLicenses.value() );
    }

  j.emplace( "allow", allow );

  return j;

}


/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & j, Preferences & p )
{
  Preferences _p( j );
  p = _p;
}

  void
to_json( nlohmann::json & j, const Preferences & p )
{
  j = p.toJSON();
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
