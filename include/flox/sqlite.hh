/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <nix/globals.hh>
#include <nix/sqlite.hh>
#include <sqlite3.h>


/* -------------------------------------------------------------------------- */

namespace flox {
  namespace sqlite {

/* -------------------------------------------------------------------------- */

class SQLiteDb : public nix::SQLite {

  public:
    SQLiteDb() : nix::SQLite() {};

    SQLiteDb(
      const std::string & path
    ,       int           flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
    ,       bool          cache = false
    ,       bool          trace = false
    , const char        * vfs   = nix::settings.useSQLiteWAL ? nullptr
                                                             : "unix-dotfile"
    );

    SQLiteDb(
      const std::string & path
    ,       bool          create = true
    ,       bool          write  = true
    ,       bool          cache  = false
    ,       bool          trace  = false
    , const char        * vfs    = nix::settings.useSQLiteWAL ? nullptr
                                                              : "unix-dotfile"
    ) : SQLiteDb( path
                , ( ( create ? SQLITE_OPEN_CREATE : 0 ) |
                    ( write  ? SQLITE_OPEN_READWRITE : SQLITE_OPEN_READONLY )
                  )
                , cache
                , trace
                , vfs
                )
    {}

};  /* End class `SQLiteDb' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::sqlite' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
