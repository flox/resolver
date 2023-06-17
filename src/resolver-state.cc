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
#include "resolve.hh"
#include "flox/util.hh"
#include <optional>
#include <vector>
#include <map>


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
  std::list<Resolved> results;
  std::list<Cursor>   todos;

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
              if ( s != nullptr ) { todos.push_back( (Cursor) s ); }
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
          if ( c != nullptr ) { todos.push_back( (Cursor) c ); }
        }
    }

  // TODO: resolve relative
  // TODO: walk
  // TODO: sort results

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
