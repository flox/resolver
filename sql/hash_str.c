/* ========================================================================== *
 *
 * Provides a simple string hashing function `hash_str( STR )'.
 *
 * Example Usage:
 *   sqlite> .load ./libsqlexts.so
 *   sqlite> SELECT hash_str( 'Hello, World!' );
 *   -6390844608310610124
 *
 *
 * -------------------------------------------------------------------------- *
 *
 * Building ( change the lib extension from `.so' to `.dylib' on Darwin :
 *   $ cc -shared -fPIC -O2 -g $( pkg-config --cflags --libs sqlite3; )  \
 *        -o libsqlexts.so ./hash_str.c;
 *
 *
 * -------------------------------------------------------------------------- *
 *
 * Borrowed from `github:aakropotin/sqlite3-exts' ( Alex Ameen ).
 *
 *
 * -------------------------------------------------------------------------- */

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include <assert.h>
#include <string.h>
#include <stdarg.h>


/* -------------------------------------------------------------------------- */

/* Fowler-Noll-Vol Hash */
  static size_t
hash_string( const unsigned char * str )
{
  size_t hash = 0x811C9DC5;
  while ( ( * str ) != '\0' )
    {
      hash ^= (unsigned char) * str;
      hash *= 0x01000193;
      str++;
    }
  return hash;
}


/* -------------------------------------------------------------------------- */

/**
 * Implementation of the hash_str( X ) function.
 *
 * Return a 64bit integer representing a unique hash for a given string.
 * The hash itself is encoded as a 64bit unsigned integer, which will appear
 * signed when converted to a `sqlite3_int64' - ignore the signedness.
 */
  static void
hashStrFunc( sqlite3_context *  context
           , int                argc
           , sqlite3_value   ** argv
           )
{
  int    eType = sqlite3_value_type( argv[0] );
  size_t hash  = 0;
  assert( argc == 1 );
  if ( eType == SQLITE_NULL ) return;
  assert( eType == SQLITE_TEXT );
  const unsigned char * str = sqlite3_value_text( argv[0] );
  hash = hash_string( str );
  sqlite3_result_int64( context, (sqlite3_int64) hash );
}


/* -------------------------------------------------------------------------- */

  int
sqlite3_extension_init( sqlite3                    *  db
                      , char                       ** pzErrMsg
                      , const sqlite3_api_routines *  pApi
                      )
{
  int rc = SQLITE_OK;
  SQLITE_EXTENSION_INIT2( pApi );
  rc = sqlite3_create_function(
         db
       , "hash_str"                                         // function name
       , 1                                                  // # args
       , SQLITE_UTF8|SQLITE_INNOCUOUS|SQLITE_DETERMINISTIC  // param encoding
       , NULL                                               // User data ptr
       , hashStrFunc                                        // xFunc fn ptr
       , NULL                                               // xStep fn ptr
       , NULL                                               // xFinal fn ptr
       );
  return rc;
}


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
