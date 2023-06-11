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
      j.emplace( "stabilities", this->stabilities );
    }

  if ( ! this->prefixes.empty() )
    {
      j.emplace( "prefixes", this->prefixes );
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

  PkgPredicate
Preferences::pred() const
{
  if ( this->allowUnfree &&
       this->allowBroken &&
       ( ! this->allowedLicenses.has_value() )
     )
    {
      return defaultPkgPredicate;
    }

  return [&](       nix::ref<nix::eval_cache::AttrCursor>   pos
            , const std::vector<nix::Symbol>              & path
            )
  {
    std::shared_ptr<nix::eval_cache::AttrCursor> mMeta =
      pos->maybeGetAttr( "meta" );
    if ( mMeta == nullptr ) { return true; }

    if ( ! this->allowUnfree )
      {
        std::shared_ptr<nix::eval_cache::AttrCursor> mUnfree =
          mMeta->maybeGetAttr( "unfree" );
        if ( ( mUnfree != nullptr ) && ( mUnfree->getBool() ) )
          {
            return false;
          }
      }

    if ( ! this->allowBroken )
      {
        std::shared_ptr<nix::eval_cache::AttrCursor> mBroken =
          mMeta->maybeGetAttr( "broken" );
        if ( ( mBroken != nullptr ) && ( mBroken->getBool() ) )
          {
            return false;
          }
      }

    if ( this->allowedLicenses.has_value() )
      {
        std::shared_ptr<nix::eval_cache::AttrCursor> mLicense =
          mMeta->maybeGetAttr( "license" );
        if ( ( mLicense != nullptr ) &&
             ( this->allowedLicenses.value().find(
                 mLicense->getAttr( "spdxId" )->getString()
               ) == this->allowedLicenses.value().end()
             )
           )
          {
            return false;
          }
      }

    return true;
  };
}


/* -------------------------------------------------------------------------- */

  void
from_json( const nlohmann::json & j, Preferences & p )
{
  p = Preferences( j );
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
