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


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

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

  std::map<std::string, std::list<Resolved>>
ResolverState::getResults() const
{
  return this->_results;
}

  void
ResolverState::clearResults()
{
  this->_results.clear();
}


/* -------------------------------------------------------------------------- */

  size_t
ResolverState::resolveInInput( std::string_view id, const Descriptor & desc )
{
  std::string _id( id );
  this->_results.erase( _id );
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
      this->_results.emplace( id, std::move( results ) );
      return 0;
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
  pred = pred && desc.pred( nix::ref<nix::SymbolTable>( ssymtab ) );

  std::queue<Package, std::list<Package>> goods;

  if ( todos.empty() )
    {
      for ( Cursor prefix : flake->getFlakePrefixCursors() )
        {
          todos.push( prefix );
        }
      while ( ! todos.empty() )
        {
          std::vector<Package> hits;
          std::vector<nix::Symbol> path = todos.front()->getAttrPath();
          for ( const nix::Symbol s : todos.front()->getAttrs() )
            {
              try
                {
                  Cursor c = todos.front()->getAttr( s );
                  if ( c->isDerivation() )
                    {
                      Package p( c, & this->getEvalState()->symbols, false );
                      if ( pred( p ) ) { hits.push_back( std::move( p ) ); }
                    }
                  else if ( this->getEvalState()->symbols[path[0]] !=
                            "packages"
                          )
                    {
                      MaybeCursor m =
                        c->maybeGetAttr( "recurseForDerivations" );
                      if ( ( m != nullptr ) && m->getBool() )
                        {
                          todos.push( (Cursor) c );
                        }
                    }
                }
              catch( ... ) {}
            }
          /* Sort new hits by version. */
          std::function<bool( const Package &, const Package & )> sortV = [](
            const Package & a
          , const Package & b
          ) {
              std::optional<std::string> va = a.getVersion();
              std::optional<std::string> vb = b.getVersion();
              if ( va.has_value() && ( ! vb.has_value() ) ) { return true; }
              if ( vb.has_value() && ( ! va.has_value() ) ) { return false; }
              /* TODO: handle pre-release tags */
              return 0 < nix::compareVersions( va.value(), vb.value() );
            };
          std::sort( hits.begin(), hits.end(), sortV );
          for ( Package & p : hits ) { goods.push( std::move( p ) ); }
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
              Package p( todos.front()
                       , & this->getEvalState()->symbols
                       , false
                       );
              if ( pred( p ) ) { goods.push( std::move( p ) ); }
            }
          todos.pop();
        }
    }

  while ( ! goods.empty() )
    {
      std::vector<nix::SymbolStr> path =
        this->getEvalState()->symbols.resolve( goods.front().getPath() );
      AttrPathGlob gp;
      for ( const nix::SymbolStr & sp : path )
        {
          gp.path.push_back( (std::string) sp );
        }
      results.push_back( Resolved(
        flake->getFlakeRef()
      , std::move( gp )
      , (nlohmann::json) { { path[1], {
          { "name",    goods.front().getFullName() }
        , { "pname",   goods.front().getPname() }
        , { "version", goods.front().getVersion().value_or( nullptr ) }
        , { "semver",  goods.front().getSemver().value_or( nullptr ) }
        , { "outputs", goods.front().getOutputs() }
        , { "license", goods.front().getLicense().value_or( nullptr ) }
        , { "broken",  goods.front().isBroken().value_or( false ) }
        , { "unfree",  goods.front().isUnfree().value_or( false ) }
        } } }
      ) );
      goods.pop();
    }

  // TODO: Sort results by version and depth.

  size_t count = results.size();
  this->_results.emplace( id, std::move( results ) );
  return count;
}


/* -------------------------------------------------------------------------- */

  }  /* End namespace `flox::resolve' */
}    /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
