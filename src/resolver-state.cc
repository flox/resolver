/* ========================================================================== *
 *
 * This is largely borrowed from `<nix>/src/libcmd/commands.hh' except we drop
 * `run' member functions, parsers, and some other unnecessary portions.
 *
 * -------------------------------------------------------------------------- */

#include <nlohmann/json.hpp>
#include <nix/eval-inline.hh>
#include <nix/eval.hh>
#include <nix/eval-cache.hh>
#include <nix/flake/flake.hh>
#include <nix/shared.hh>
#include <nix/store-api.hh>
#include "resolve.hh"
#include "flox/util.hh"
#include <optional>
#include <vector>
#include <map>
#include "flox/predicates.hh"
#include <queue>
#include "flox/drv-cache.hh"
#include "flox/eval-package.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

ResolverState::ResolverState(
  const Inputs                 & inputs
, const Preferences            & prefs
, const std::list<std::string> & systems
)
: _prefs( prefs )
{
  nix::initNix();
  nix::initGC();
  // TODO: make this an option. It risks making cross-system eval impossible.
  nix::evalSettings.enableImportFromDerivation.setDefault( false );
  nix::evalSettings.pureEval.setDefault( true );
  nix::evalSettings.useEvalCache.setDefault( true );

  for ( auto & [id, ref] : inputs.inputs )
    {
#if HAVE_BOEHMGC
      this->_inputs.emplace( id, std::allocate_shared<FloxFlake>(
        traceable_allocator<FloxFlake>()
      , this->getEvalState()
      , id
      , ref
      , this->_prefs
      , systems
      ) );
#else
      this->_inputs.emplace( id, std::make_shared<FloxFlake>(
        this->getEvalState()
      , id
      , ref
      , this->_prefs
      , systems
      ) );
#endif
    }
}


/* -------------------------------------------------------------------------- */

  nix::ref<nix::Store>
ResolverState::getStore()
{
  if ( this->_store == nullptr )
    {
      this->_store = nix::openStore();
    }
  return nix::ref<nix::Store>( this->_store );
}


/* -------------------------------------------------------------------------- */

  nix::ref<nix::Store>
ResolverState::getEvalStore()
{
  if ( this->evalStore == nullptr )
    {
      this->evalStore = this->getStore();
    }
  return nix::ref<nix::Store>( this->evalStore );
}


/* -------------------------------------------------------------------------- */

  nix::ref<nix::EvalState>
ResolverState::getEvalState()
{
  if ( this->evalState == nullptr )
    {
#if HAVE_BOEHMGC
      this->evalState = std::allocate_shared<nix::EvalState>(
        traceable_allocator<nix::EvalState>()
      , std::list<std::string> {}
      , this->getEvalStore()
      , this->getStore()
      );
#else
      this->evalState = std::make_shared<nix::EvalState>(
        std::list<std::string> {}
      , this->getEvalStore()
      , this->getStore()
      );
#endif
      this->evalState->repair = nix::NoRepair;
    }
  return nix::ref<nix::EvalState>( this->evalState );
}


/* -------------------------------------------------------------------------- */

  std::list<std::string>
ResolverState::getInputNames() const
{
  std::list<std::string> names;
  for ( const auto & [id, flake] : this->_inputs )
    {
      names.push_back( id );
    }
  return names;
}


/* -------------------------------------------------------------------------- */

  std::map<std::string, nix::ref<FloxFlake>>
ResolverState::getInputs() const
{
  std::map<std::string, nix::ref<FloxFlake>> rsl;
  for ( auto & [id, flake] : this->_inputs )
    {
      rsl.emplace( id, nix::ref<FloxFlake>( flake ) );
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  std::optional<nix::ref<FloxFlake>>
ResolverState::getInput( std::string_view id ) const
{
  for ( auto & [_id, flake] : this->_inputs )
    {
      if ( _id == id  ) { return nix::ref<FloxFlake>( flake ); }
    }
  return std::nullopt;
}


/* -------------------------------------------------------------------------- */

  std::list<Resolved>
ResolverState::resolveInInput( std::string_view id, const Descriptor & desc )
{
  std::string                           _id( id );
  std::list<Resolved>                   results;
  std::queue<Cursor, std::list<Cursor>> todos;

  /**
   * 1. Handling of `id' should have already been handled elsewhere.
   * 2. If we have an `absAttrPath' with no glob we can avoid traversal or
   *    iteration over subtrees and systems.
   * 3. If we have a `relAttrPath' we can avoid a full traversal and just
   *    iterate over subtrees and systesm.
   * 4. Otherwise we have to do a full traversal.
   *    The `packages' output can be optimized slightly by avoiding recursive
   *    descent into `recurseForDerivations' attrs.
   */

  /* Bail early if `id' isn't a match.
   * This should have already been handled by the caller but this will clear
   * existing results in case they do call with a bad input. */
  if ( desc.inputId.has_value() && ( id != desc.inputId.value() ) )
    {
      return {};
    }

  std::shared_ptr<FloxFlake> flake = this->_inputs.at( _id );

  /* Handle `absAttrPath' */
  if ( desc.absAttrPath.has_value() )
    {
      if ( desc.absAttrPath.value().hasGlob() )
        {
          std::vector<nix::Symbol> subtree = {
            this->getEvalState()->symbols.create(
              std::get<std::string>( desc.absAttrPath.value().path[0] )
            )
          };
          Cursor c = flake->openCursor( subtree );
          for ( const std::string & system : flake->getSystems() )
            {
              MaybeCursor s = c->maybeGetAttr( system );
              for ( size_t i = 2;
                    i < desc.absAttrPath.value().path.size();
                    ++i
                  )
                {
                  if ( s == nullptr ) { break; }
                  s = s->maybeGetAttr(
                    std::get<std::string>( desc.absAttrPath.value().path[i] )
                  );
                }
              if ( s != nullptr ) { todos.push( (Cursor) s ); }
            }
        }
      else
        {
          std::vector<nix::Symbol> path;
          for ( size_t i = 0; i < desc.absAttrPath.value().path.size(); ++i )
            {
              this->getEvalState()->symbols.create(
                std::get<std::string>( desc.absAttrPath.value().path[i] )
              );
            }
          MaybeCursor c = flake->maybeOpenCursor( path );
          if ( c != nullptr ) { todos.push( (Cursor) c ); }
        }
    }
  else if ( desc.relAttrPath.has_value() )
    {
      for ( Cursor c : flake->getFlakePrefixCursors() )
        {
          MaybeCursor m = c;
          for ( const std::string & p : desc.relAttrPath.value() )
            {
              if ( m == nullptr ) { break; }
              m = m->getAttr( p );
            }
          if ( m != nullptr ) { todos.push( (Cursor) m ); }
        }
    }

  std::shared_ptr<nix::SymbolTable> ssymtab =
    std::make_shared<nix::SymbolTable>( this->getEvalState()->symbols );

  predicates::PkgPred pred = this->_prefs.pred_V2();
  pred = pred && desc.pred( nix::ref<nix::SymbolTable>( ssymtab )
                          , todos.empty()
                          );

  std::queue<Package *, std::list<Package *>> goods;

  if ( todos.empty() )
    {
      DrvDb cache( flake->getLockedFlake()->getFingerprint() );

      for ( Cursor prefix : flake->getFlakePrefixCursors() )
        {
          std::vector<nix::SymbolStr> ppath =
            this->getEvalState()->symbols.resolve( prefix->getAttrPath() );
          if ( ( ! desc.searchCatalogs ) && ( ppath[0] == "catalog" ) )
            {
              continue;
            }
          else if ( desc.catalogStability.has_value() &&
                    ( desc.catalogStability.value() != ppath[2] )
                  )
            {
              continue;
            }
          if ( ( ! desc.searchFlakes ) &&
               ( ( ppath[0] == "legacyPackages" ) ||
                 ( ppath[0] == "packages" )
               )
             )
            {
              continue;
            }
          todos.push( prefix );
        }
      while ( ! todos.empty() )
        {
          std::vector<nix::Symbol> path = todos.front()->getAttrPath();

          std::string subtree         = this->getEvalState()->symbols[path[0]];
          std::string system          = this->getEvalState()->symbols[path[1]];
          DrvDb::progress_status dbps = cache.getProgress( subtree, system );
          if ( dbps < DrvDb::progress_status::DBPS_PARTIAL )
            {
              cache.setProgress(
                subtree, system, DrvDb::progress_status::DBPS_PARTIAL
              );
            }

          for ( const nix::Symbol s : todos.front()->getAttrs() )
            {
              try
                {
                  Cursor c = todos.front()->getAttr( s );
                  if ( c->isDerivation() )
                    {
                      // TODO: load from cache if available
                      EvalPackage * p = new EvalPackage(
                        c, & this->getEvalState()->symbols, false
                      );
                      if ( dbps < DrvDb::progress_status::DBPS_INFO_DONE )
                        {
                          cache.setDrvInfo( (Package &) * p );
                        }
                      if ( pred( (Package &) * p ) ) { goods.push( p ); }
                    }
                  else if ( subtree != "packages" )
                    {
                      MaybeCursor m =
                        c->maybeGetAttr( "recurseForDerivations" );
                      if ( ( m != nullptr ) && m->getBool() )
                        {
                          todos.push( (Cursor) c );
                        }
                    }
                }
              catch( ... )
                {
                  // TODO: Catch errors in `packages'.
                }
            }
          if ( dbps < DrvDb::progress_status::DBPS_INFO_DONE )
            {
              cache.setProgress(
                subtree, system, DrvDb::progress_status::DBPS_INFO_DONE
              );
            }
          todos.pop();
        }
    }
  else
    {
      /* Run our predicate filters and collect satisfactory packages. */
      while( ! todos.empty() )
        {
          if ( todos.front()->isDerivation() )
            {
              EvalPackage * p = new EvalPackage( todos.front()
                                               , & this->getEvalState()->symbols
                                               , false
                                               );
              if ( pred( (Package &) * p ) ) { goods.push( p ); }
            }
          todos.pop();
        }
    }

  while ( ! goods.empty() )
    {
      results.push_back( Resolved(
        flake->getFlakeRef()
      , AttrPathGlob::fromStrings( goods.front()->getPathStrs() )
      , goods.front()->getInfo()
      ) );
      delete goods.front();
      goods.pop();
    }

  // TODO: Merge results by glob path
  // TODO: Sort results by version and depth.

  return results;
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
