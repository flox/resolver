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
  MaybeCursor curr = this->openCursor();
  if ( curr == nullptr ) { return false; }
  try
    {
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
  MaybeCursor curr = this->openCursor();
  if ( curr == nullptr ) { return 0; }
  if ( this->_subtree == ST_PACKAGES ) { return curr->getAttrs().size(); }
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
  MaybeCursor curr = this->openCursor();
  todo_queue todo;
  todo.emplace( std::move( curr ) );
  return const_iterator( this->_subtree
                       , & this->_state->symbols
                       , std::move( todo )
                       );
}


/* -------------------------------------------------------------------------- */

  bool
FlakePackageSet::const_iterator::evalPackage()
{
  if ( ( this->_todo.empty() ) || ( this->_it == this->_end ) )
    {
      this->_ptr = nullptr;
      return false;
    }
  Cursor c = this->_todo.front();
  try
    {
      c = c->getAttr( * this->_it );
      if ( ( this->_subtree == ST_PACKAGES ) || c->isDerivation() )
        {
          this->_ptr = std::make_shared<FlakePackage>(
            c
          , this->_symtab
          , false          /* checkDrv */
          );
          return true;
        }
    }
  catch( ... ) {}
  this->_ptr = nullptr;
  return false;
}



/* -------------------------------------------------------------------------- */


  FlakePackageSet::const_iterator &
FlakePackageSet::const_iterator::operator++()
{
  /* We are "seeking" for packages using a queue of child iterator that loops
   * over attributes.
   * We use `goto' to jump back up to this point until we either hit the end
   * of our iterator queue, or find a package. */
  recur:
    /* If we've reached the end of our search, mark a phony sentinel value.
     * That is "we fill phony values" that are recognized as a marker. */
    if ( this->_todo.empty() ) { return this->clear(); }

    /* Go to the next attribute in our current iterator. */
    ++this->_it;

    /* If we hit the end either start processing the next `todo' list member,
     * or bail if it's empty. */
    if ( this->_it == this->_end )
      {
        this->_todo.pop();
        if ( this->_todo.empty() )
          {
            /* Set to sentinel value and bail. */
            return this->clear();
          }
        else
          {
            /* Start processing the next cursor. */
            this->_end = this->_todo.front()->getAttrs().end();
            this->_it  = this->_todo.front()->getAttrs().begin();
          }
      }

    /* See if we have a package, or if we might need to recurse into a
     * sub-attribute for more packages ( this only occurs for some subtrees ) */
    try
      {
        Cursor c = this->_todo.front()->getAttr( * this->_it );
        if ( ( this->_subtree == ST_PACKAGES ) || ( c->isDerivation() ) )
          {
            evalPackage();  /* Load the package at cursor. */
            return * this;
          }
        MaybeCursor m = c->maybeGetAttr( "recurseForDerivations" );
        if ( ( m != nullptr ) && m->getBool() )
          {
            this->_todo.push( (Cursor) c );
          }
        /* Keep searching. */
        goto recur;
      }
    catch( ... )
      {
        /* Keep searching. */
        goto recur;
      }

    throw ResolverException(
      "FlakePackageSet::const_iterator::operator++(): "
      "Readched ALLEGEDLY unreachable block."
    );
    return * this;
}  /* End `FlakePackageSet::const_iterator::operator++()' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
