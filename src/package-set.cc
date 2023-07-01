/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/package-set.hh"
#include "flox/drv-cache.hh"
#include "flox/util.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace resolve {

/* -------------------------------------------------------------------------- */

  std::string
FlakeRefWithPath::toString() const
{
  std::string rsl = this->ref.to_string();
  if ( this->path.empty() ) { return rsl; }
  rsl += "#";
  bool first = true;
  for ( const auto & p : this->path )
    {
      if ( first ) { rsl += "\"" + p + "\""; first = false; }
      else         { rsl += ".\"" + p + "\""; }
    }
  return rsl;
}

  std::string
to_string( const FlakeRefWithPath & rp )
{
  return rp.toString();
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::resolve' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
