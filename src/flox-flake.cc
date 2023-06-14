/* ========================================================================== *
 *
 * This is largely borrowed from `<nix>/src/libcmd/commands.hh' except we drop
 * `run' member functions, parsers, and some other unnecessary portions.
 *
 * -------------------------------------------------------------------------- */

#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include <optional>
#include <vector>
#include <map>
#include <memory>
#include "flox/types.hh"
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

FloxFlake::FloxFlake(       nix::ref<nix::EvalState>   state
                    ,       std::string_view           id
                    , const FloxFlakeRef             & ref
                    , const Preferences              & prefs
                    , const std::list<std::string>   & systems
                    )
  : _state( state )
  , _flakeRef( nix::FlakeRef::fromAttrs( ref.toAttrs() ) )
  , _systems( systems )
  , _prefsStabilities(
      ( prefs.stabilities.find( std::string( id ) ) != prefs.stabilities.end() )
      ? prefs.stabilities.at( std::string( id ) )
      : defaultCatalogStabilities
    )
  , _prefsPrefixes(
      ( prefs.prefixes.find( std::string( id ) ) != prefs.prefixes.end() )
      ? prefs.prefixes.at( std::string( id ) )
      : defaultAttrPathPrefixes
    )
{
}


/* -------------------------------------------------------------------------- */

  std::shared_ptr<nix::flake::LockedFlake>
FloxFlake::getLockedFlake()
{
  if ( this->lockedFlake == nullptr )
    {
      this->lockedFlake = std::make_shared<nix::flake::LockedFlake>(
        nix::flake::lockFlake( * this->_state
                             , this->_flakeRef
                             , floxFlakeLockFlags
                             )
      );
  }
  return this->lockedFlake;
}


/* -------------------------------------------------------------------------- */

  std::list<std::string>
FloxFlake::getSystems() const
{
  return this->_systems;
}


/* -------------------------------------------------------------------------- */

/* TODO: check additional preferences. */
  std::list<std::list<std::string>>
FloxFlake::getDefaultFlakeAttrPaths() const
{
  std::string system = nix::settings.thisSystem.get();
  if ( hasElement( this->_systems, system ) &&
       hasElement<std::string, std::vector>( this->_prefsPrefixes, "packages" )
     )
    {
      return {
        { "packages", system, "default" }
      , { "defaultPackage", std::move( system ) }
      };
    }
  return {};
}


/* -------------------------------------------------------------------------- */

  std::list<std::list<std::string>>
FloxFlake::getDefaultFlakeAttrPathPrefixes() const
{
  std::string system = nix::settings.thisSystem.get();
  if ( ! hasElement( this->_systems, system ) )
    {
      return {};
    }
  std::list<std::list<std::string>> rsl;
  for ( auto & prefix : this->_prefsPrefixes )
    {
      if ( prefix == "catalog" )
        {
          for ( auto & stability : this->_prefsStabilities )
            {
              rsl.push_back( { "catalog", system, stability } );
            }
        }
      else
        {
          rsl.push_back( { prefix, system } );
        }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::list<std::list<std::string>>
FloxFlake::getFlakeAttrPathPrefixes() const
{
  std::list<std::list<std::string>> rsl;
  for ( auto & prefix : this->_prefsPrefixes )
    {
      for ( auto & system : this->_systems )
        {
          if ( prefix == "catalog" )
            {
              for ( auto & stability : this->_prefsStabilities )
                {
                  rsl.push_back( { "catalog", system, stability } );
                }
            }
          else
            {
              rsl.push_back( { prefix, system } );
            }
        }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

   nix::ref<nix::eval_cache::EvalCache>
FloxFlake::openEvalCache()
{
  nix::flake::Fingerprint fingerprint =
    this->getLockedFlake()->getFingerprint();
  nix::ref<nix::EvalState>                 lState = this->_state;
  std::shared_ptr<nix::flake::LockedFlake> lFlake = this->getLockedFlake();
  return nix::make_ref<nix::eval_cache::EvalCache>(
    ( nix::evalSettings.useEvalCache && nix::evalSettings.pureEval )
    ? std::optional { std::cref( fingerprint ) }
    : std::nullopt
  , * this->_state
  , [&lState, lFlake]() { return loadFlakeRoot( lState, lFlake ); }
  );
}


/* -------------------------------------------------------------------------- */

  Cursor
FloxFlake::openCursor( const std::vector<nix::Symbol> & path )
{
  Cursor cur = this->openEvalCache()->getRoot();
  for ( const nix::Symbol & p : path ) { cur = cur->getAttr( p ); }
  return cur;
}


/* -------------------------------------------------------------------------- */

  MaybeCursor
FloxFlake::maybeOpenCursor( const std::vector<nix::Symbol> & path )
{
  MaybeCursor cur = this->openEvalCache()->getRoot();
  for ( const nix::Symbol & p : path )
    {
      cur = cur->maybeGetAttr( p );
      if ( cur == nullptr ) { break; }
    }
  return cur;
}


/* -------------------------------------------------------------------------- */

  std::list<Cursor>
FloxFlake::getFlakePrefixCursors()
{
  std::list<Cursor>                    rsl;
  nix::ref<nix::eval_cache::EvalCache> cache = this->openEvalCache();
  for ( std::list<std::string> & prefix : this->getFlakeAttrPathPrefixes() )
    {
      MaybeCursor cur = cache->getRoot();
      for ( std::string & p : prefix )
        {
          cur = cur->maybeGetAttr( p );
          if ( cur == nullptr ) { break; }
        }
      if ( cur != nullptr ) { rsl.push_back( Cursor( cur ) ); }
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::list<std::vector<nix::Symbol>>
FloxFlake::getActualFlakeAttrPathPrefixes()
{
  std::list<std::vector<nix::Symbol>>  rsl;
  for ( const Cursor c : this->getFlakePrefixCursors() )
    {
      rsl.push_back( c->getAttrPath() );
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

