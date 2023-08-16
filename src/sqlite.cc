/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include "flox/sqlite.hh"
#include "flox/exceptions.hh"


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace sqlite {

/* -------------------------------------------------------------------------- */

  static void
traceSQL( void *, const char * sql )
{
  /* Unrolled from of `nix::notice()'. */
  if ( nix::lvlNotice <= nix::verbosity )
    {
      nix::logger->log( nix::lvlNotice, nix::fmt( "SQL<[%1%]>", sql ) );
    }
};

/* -------------------------------------------------------------------------- */

SQLiteDb::SQLiteDb( const std::string & path
                  ,       int           flags
                  ,       bool          cache
                  ,       bool          trace
                  , const char        * vfs
                  )
{
  int ret = sqlite3_open_v2( path.c_str(), & this->db, flags, vfs );
  if ( ret != SQLITE_OK )
    {
      const char * err = sqlite3_errstr( ret );
      throw resolve::ResolverException(
        "cannot open SQLite database '" + path + "': " + std::string( err )
      );
    }

  if ( sqlite3_busy_timeout( db, 60 * 60 * 1000 ) != SQLITE_OK )
    {
      nix::SQLiteError::throw_( db, "setting timeout" );
    }

  if ( trace ) { sqlite3_trace( db, & traceSQL, nullptr ); }

  this->exec( "pragma foreign_keys = 1" );

  if ( cache ) { this->isCache(); }
}


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::sqlite' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
