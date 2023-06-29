/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/types.hh"
#include "flox/eval-package.hh"
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

  PackageSet::iterator
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
  return flake_iterator( std::move( todo ) );
}

  PackageSet::const_iterator
FlakePackageSet::begin() const
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
  return flake_const_iterator( std::move( todo ) );
}


  template<bool IS_CONST>
  FlakePackageSet::flake_iterator_impl<IS_CONST> &
FlakePackageSet::flake_iterator_impl<IS_CONST>::operator++()
{
  recur:
    if ( this->_todo.empty() ) { return this->end(); }
    ++this->it;
    if ( this->it == this->end )
      {
        this->todo.pop();
        if ( this->todo.empty() ) { return this->end(); }
      }
    else
      {
        this->end = this->todo.front()->getAttrs().end();
        this->it  = this->todo.front()->getAttrs().begin();
        try
          {
            Cursor c = this->todo.front()->getAttr( * this->it );
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
                        this->todo.push( (Cursor) c );
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
      "FlakePackageSet::flake_iterator_impl::operator++(): "
      "Readched ALLEGEDLY unreachable block."
    );
    return * this;
}  /* End `FlakePackageSet::flake_iterator::operator++()' */


  PackageSet::iterator
FlakePackageSet::end()
{
  return FlakePackageSet::flake_iterator();
}

  PackageSet::const_iterator
FlakePackageSet::end() const
{
  return FlakePackageSet::flake_const_iterator();
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
