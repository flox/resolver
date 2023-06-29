/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/types.hh"
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

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
