/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/types.hh"
#include "flox/flake-package.hh"
#include "flox/flake-package-set.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  bool
FlakePackageSet::hasRelPath( const std::list<std::string_view> & path )
{
  try
    {
      MaybeCursor curr = this->openEvalCache()->getRoot();
      curr = curr->maybeGetAttr( subtreeTypeToString( this->_subtree ) );
      if ( curr == nullptr ) { return false; }
      curr = curr->maybeGetAttr( this->_system );
      if ( curr == nullptr ) { return false; }
      if ( this->_stability.has_value() )
        {
          curr = curr->maybeGetAttr( this->_stability.value() );
          if ( curr == nullptr ) { return false; }

        }
      for ( auto & p : path )
        {
          curr = curr->maybeGetAttr( p );
          if ( curr == nullptr ) { return false; }
        }
      return curr->isDerivation();
    }
  catch( ... )
    {
      return false;
    }
  return true;
}


/* -------------------------------------------------------------------------- */

  std::size_t
FlakePackageSet::size()
{
  if ( this->_subtree == ST_PACKAGES )
    {
      MaybeCursor curr =
        this->openEvalCache()->getRoot()->maybeGetAttr( "packages" );
      if ( curr == nullptr ) { return 0; }
      curr = curr->maybeGetAttr( this->_system );
      if ( curr == nullptr ) { return 0; }
      return curr->getAttrs().size();
    }

  MaybeCursor curr = this->openEvalCache()->getRoot();
  curr = curr->maybeGetAttr( subtreeTypeToString( this->_subtree ) );
  if ( curr == nullptr ) { return 0; }
  curr = curr->maybeGetAttr( this->_system );
  if ( curr == nullptr ) { return 0; }
  if ( this->_stability.has_value() )
    {
      curr = curr->maybeGetAttr( this->_stability.value() );
      if ( curr == nullptr ) { return 0; }
    }

  std::size_t rsl = 0;
  todo_queue todos;
  todos.push( (Cursor) std::move( curr ) );
  while ( ! todos.empty() )
    {
      for ( const nix::Symbol s : todos.front()->getAttrs() )
        {
          try
            {
              Cursor c = todos.front()->getAttr( s );
              if ( c->isDerivation() )
                {
                  ++rsl;
                }
              else
                {
                  MaybeCursor m = c->maybeGetAttr( "recurseForDerivations" );
                  if ( ( m != nullptr ) && m->getBool() )
                    {
                      todos.push( (Cursor) c );
                    }
                }
            }
          catch( ... )
            {
              // If eval fails ignore the package.
              nix::ignoreException();
            }
        }
      todos.pop();
    }
  return rsl;
}


/* -------------------------------------------------------------------------- */

  FlakePackageSet::const_iterator
FlakePackageSet::begin()
{
  MaybeCursor curr = this->openEvalCache()->getRoot();
  if ( this->_subtree == ST_PACKAGES )
    {
      curr = curr->maybeGetAttr( "packages" );
      if ( curr == nullptr ) { return this->end(); }
      curr = curr->maybeGetAttr( this->_system );
      if ( curr == nullptr ) { return this->end(); }
    }
  else
    {
      curr = curr->maybeGetAttr( subtreeTypeToString( this->_subtree ) );
      if ( curr == nullptr ) { return this->end(); }
      curr = curr->maybeGetAttr( this->_system );
      if ( curr == nullptr ) { return this->end(); }
      if ( this->_stability.has_value() )
        {
          curr = curr->maybeGetAttr( this->_stability.value() );
          if ( curr == nullptr ) { return this->end(); }
        }
    }
  todo_queue todo;
  todo.emplace( std::move( curr ) );
  return const_iterator( this->_subtree, std::move( todo ) );
}


  FlakePackageSet::const_iterator &
FlakePackageSet::const_iterator::operator++()
{
  recur:
    if ( this->_todo.empty() )
      {
        this->_end = std::vector<nix::Symbol>().cend();
        this->_it  = std::vector<nix::Symbol>().begin();
        return * this;
      }
    ++this->_it;
    if ( this->_it == this->_end )
      {
        this->_todo.pop();
        if ( this->_todo.empty() )
          {
            this->_end = std::vector<nix::Symbol>().cend();
            this->_it  = std::vector<nix::Symbol>().begin();
            return * this;
          }
      }
    else
      {
        this->_end = this->_todo.front()->getAttrs().end();
        this->_it  = this->_todo.front()->getAttrs().begin();
        try
          {
            Cursor c = this->_todo.front()->getAttr( * this->_it );
            if ( this->_subtree == ST_PACKAGES )
              {
                return * this;
              }
            else
              {
                if ( c->isDerivation() )
                  {
                    return * this;
                  }
                else
                  {
                    MaybeCursor m = c->maybeGetAttr( "recurseForDerivations" );
                    if ( ( m != nullptr ) && m->getBool() )
                      {
                        this->_todo.push( (Cursor) c );
                      }
                    goto recur;
                  }
              }
          }
        catch( ... )
          {
            goto recur;
          }
      }
    throw ResolverException(
      "FlakePackageSet::const_iterator::operator++(): "
      "Readched ALLEGEDLY unreachable block."
    );
    return * this;
}  /* End `FlakePackageSet::const_iterator::operator++()' */


  FlakePackageSet::const_iterator
FlakePackageSet::end() const
{
  return const_iterator();
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
