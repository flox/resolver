/* ========================================================================== *
 *
 * @file flox/sqlite.hh
 *
 * @brief Extensions to the `nix` SQLite3 handle/interface.
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

/**
 * Handle used to interact with a SQLite3 database.
 * This extend's the @a nix::SQLite handle to allow read-only and read/write
 * modes to be used, along with arbitrary `SQLITE_*` flags.
 */
class SQLiteDb : public nix::SQLite {

  public:
    SQLiteDb() : nix::SQLite() {};  /**< Constructs a dummy db handle. */

    /**
     * Opens a database handle with arbitrary `SQLITE_*` flags.
     * @param path Absolute path to a SQLite3 database.
     * @param flags `SQLITE_*` flags to pass to `sqlite_open`.
     * @param cache Whether the database should disable synchronization.
     * @param trace Whether verbose debug traces should be emitted.
     * @param vfs Locking policy to use for SQLite3 database
     *            ( ignored if _Write Ahead Logging_ is enabled ).
     */
    SQLiteDb(
      const std::string & path
    ,       int           flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
    ,       bool          cache = false
    ,       bool          trace = false
    , const char        * vfs   = nix::settings.useSQLiteWAL ? nullptr
                                                             : "unix-dotfile"
    );

    /**
     * Opens a database handle.
     * @param path Absolute path to a SQLite3 database.
     * @param create Whether a database should be created if one does not exist.
     * @param cache Whether the database should disable synchronization.
     * @param trace Whether verbose debug traces should be emitted.
     * @param vfs Locking policy to use for SQLite3 database
     *            ( ignored if _Write Ahead Logging_ is enabled ).
     */
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
