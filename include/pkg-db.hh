/* ========================================================================== *
 *
 * @file pkg-db.hh
 *
 * @brief Interfaces for operating on a SQLite3 package set database.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <ranges>
#include <nlohmann/json.hpp>
#include <nix/flake/flake.hh>
#include <nix/eval-cache.hh>

#include "sqlite3pp.hh"
#include "flox/raw-package.hh"


/* -------------------------------------------------------------------------- */

#define FLOX_PKGDB_SCHEMA_VERSION  "0.1.0"


/* -------------------------------------------------------------------------- */

namespace flox {

  /** Interfaces for caching package metadata in SQLite3 databases. */
  namespace pkgdb {

/* -------------------------------------------------------------------------- */

using Fingerprint = nix::flake::Fingerprint;
using SQLiteDb    = sqlite3pp::database;
using AttrPath    = std::vector<std::string_view>;
using pkg_id      = int;


/* -------------------------------------------------------------------------- */

/** Get an absolute path to the `PkgDb' for a given fingerprint hash. */
std::string getPkgDbName( const Fingerprint & fingerprint );

/** Get an absolute path to the `PkgDb' for a locked flake. */
  static inline std::string
getPkgDbName( const nix::flake::LockedFlake & flake )
{
  return getPkgDbName( flake.getFingerprint() );
}


/* -------------------------------------------------------------------------- */

/**
 * Construct a new `resolve::RawPackage' from a SQLite row.
 *
 * The following columns are required:
 *   `parent`, `attrName`, `name`, `pname`, `version`, `semver`, `license`,
 *   `outputs, `outputsToInstall`, `broken`, and `unfree`.
 *
 * @param row A single SQLite row from the `Packages' table.
 * @return A `resolve::RawPackage` object filled with @a row data.
 * @see PkgDb::getPackages( const AttrPath & path )
 */
resolve::RawPackage packageFromRow( sqlite3pp::query::rows row );


/* -------------------------------------------------------------------------- */

/**
 * A SQLite3 database used to cache derivation/package information about a
 * single locked flake.
 */
class PkgDb {

  /* Data */

  private:

    /** Create tables in database if they do not exist. */
    void initTables();

    /** Set @a this `PkgDb` `lockedRef` fields from database metadata. */
    void loadLockedRef();


  public:

          SQLiteDb    db;           /**< SQLite3 database handle.          */
    const Fingerprint fingerprint;  /**< Unique hash of associated flake.  */

    /** Locked _flake reference_ for database's flake. */
    const struct {
      std::string    string;  /**< Locked URI string.  */
      nlohmann::json attrs;   /**< Exploded form of URI as an attr-set. */
    } lockedRef;


/* -------------------------------------------------------------------------- */

  /* Constructors */

  public:
    PkgDb() : fingerprint( nix::htSHA256 ) {}

    /** Opens a DB associated with a locked flake. */
    PkgDb( const nix::flake::LockedFlake & flake );

    /** Opens a DB directly by its fingerprint hash. */
    PkgDb( const Fingerprint & fingerprint );


/* -------------------------------------------------------------------------- */

  /* Basic Operations */

  public:

    /** @return The Package Database schema version. */
    std::string getDbVersion();

    /**
     * Execute a raw sqlite statement on the database.
     * @param stmt String statement to execute.
     * @return `SQLITE_*` [error code](https://www.sqlite.org/rescode.html).
     */
     int execute( std::string_view stmt );


/* -------------------------------------------------------------------------- */

  /* Queries */

  public:

    /**
     * Check to see if database has packages under the attribute path
     * prefix @a path.
     * @param path An attribute path prefix such as `packages.x86_64-linux` or
     *             `legacyPackages.aarch64-darwin.python3Packages`.
     * @return `true` iif the database has one or more rows in the `Packages`
     *         table with `path` as the _parent_.
     */
    bool hasPackageSet( const AttrPath & path );

    /**
     * Check to see if database has a package at the attribute path @a path.
     * @param path An attribute path such as `packages.x86_64-linux.hello` or
     *             `legacyPackages.aarch64-darwin.python3Packages.pip`.
     * @return `true` iif the database has a rows in the `Packages`
     *         table with `path` as the _absolute path_.
     */
    bool hasPackage( const AttrPath & path );


    /**
     * Iterate over a package set.
     * @param path An attribute path prefix such as `packages.x86_64-linux` or
     *             `legacyPackages.aarch64-darwin.python3Packages`.
     * @return An iterator of `Package` objects.
     */
    // std::ranges::transform_view<
    //   std::ranges::ref_view<sqlite3pp::query>
    // , flox::resolve::RawPackage (*)(sqlite3pp::query::rows)
    // >
      auto
    getPackages( const AttrPath & path )
    {
      sqlite3pp::query q(
        this->db
      , R"SQL(
          SELECT ( :parent AS parent, attrName, name, pname, version, semver
                 , license, outputs, outputsToInstall, broken, unfree
                 )
          FROM Packages INNER JOIN Packages.parentId = PackageSets.pathId
          WHERE path = :parent
        )SQL"
      );
      q.bind( ":parent"
            , nlohmann::json( path ).dump().c_str()
            , sqlite3pp::nocopy
            );
      return std::ranges::views::transform( q, packageFromRow );
    }


    std::unique_ptr<resolve::RawPackage> getPackage( const AttrPath & path );


/* -------------------------------------------------------------------------- */

  /* Insert */

  public:
    /**
     * Adds a package to the database.
     * @param pkg The package to be added.
     * @param replace Whether to replace/ignore existing rows.
     * @return The `Packages.id` value for the added package.
     */
    pkg_id addPackage( resolve::Package & pkg, bool replace = false );



/* -------------------------------------------------------------------------- */

};  /* End class `PkgDb' */


/* -------------------------------------------------------------------------- */

  }  /* End Namespace `flox::pkgdb' */
}  /* End Namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */
