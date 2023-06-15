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

  int
Preferences::compareInputs(
  const std::string_view idA, const FloxFlakeRef & a
, const std::string_view idB, const FloxFlakeRef & b
) const
{
  if ( ( idA == idB ) && ( a == b ) ) { return 0; }
  int rankA = this->inputs.size();
  int rankB = this->inputs.size();
  for ( int i = 0; i < this->inputs.size(); ++i )
    {
      if ( this->inputs[i] == idA ) { rankA = i; }
      if ( this->inputs[i] == idB ) { rankB = i; }
    }

  if ( rankA < rankB ) { return -1; }
  if ( rankB < rankA ) { return 1; }
  if ( a == b )        { return 0; }

  /* Try to compare by input `revCount' or `lastModifiedDate'. */
  const nix::fetchers::Input & ia = a.input;
  const nix::fetchers::Input & ib = b.input;

  /* Prefer locked inputs. */
  // TODO: make this an option
  if ( ia.isLocked() && ( ! ib.isLocked() ) )      { return -1; }
  else if ( ! ia.isLocked() && ( ib.isLocked() ) ) { return 1; }

  std::optional<uint64_t> lmA = ia.getLastModified();
  std::optional<uint64_t> lmB = ib.getLastModified();
  if ( lmA.has_value() && lmB.has_value() )
    {
      if ( lmA.value() < lmB.value() )      { return -1; }
      else if ( lmB.value() < lmA.value() ) { return 1; }
      else                                  { return 0; }
    }
  /* Sort lexicographically by `id' to break ties. */
  if ( idA < idB ) { return -1; }
  if ( idB < idA ) { return 1; }
  return 0;
}


/* -------------------------------------------------------------------------- */

  nlohmann::json
Preferences::toJSON() const
{
  nlohmann::json j;
  if ( ! this->inputs.empty() ) { j.emplace( "inputs", this->inputs ); }

  if ( ! this->stabilities.empty() )
    {
      j.emplace( "stabilities", this->stabilities );
    }

  if ( ! this->prefixes.empty() ) { j.emplace( "prefixes", this->prefixes ); }

  j.emplace( "semver", (nlohmann::json) {
    { "preferPreReleases", this->semverPreferPreReleases }
  } );

  nlohmann::json allow = {
    { "broken", this->allowBroken }
  , { "unfree", this->allowUnfree }
  };

  if ( this->allowedLicenses.has_value() )
    {
      allow.emplace( "licenses", this->allowedLicenses.value() );
    }

  j.emplace( "allow", std::move( allow ) );

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

  predicates::PkgPred
Preferences::pred_V2() const
{
  if ( this->allowUnfree &&
       this->allowBroken &&
       ( ! this->allowedLicenses.has_value() )
     )
    {
      return predicates::predTrue;
    }
  predicates::PkgPred pred = predicates::hasMeta;
  if ( ! this->allowUnfree ) { pred = pred && predicates::isFree;    }
  if ( ! this->allowBroken ) { pred = pred && predicates::notBroken; }
  if ( this->allowedLicenses.has_value() )
    {
      std::vector<std::string> ls;
      for ( const auto & l : this->allowedLicenses.value() )
        {
          ls.push_back( l );
        }
      pred = pred && predicates::hasLicense( ls );
    }
  return pred;
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
