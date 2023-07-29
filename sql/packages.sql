-- ========================================================================== --
--
--
--
-- -------------------------------------------------------------------------- --

CREATE TABLE IF NOT EXISTS PackageSets (
  path  JSON  NOT NULL PRIMARY KEY
);


-- -------------------------------------------------------------------------- --

INSERT OR REPLACE INTO PackageSets VALUES
-- Standard
  ('["packages","x86_64-linux"]')
, ('["packages","x86_64-darwin"]')
, ('["packages","aarch64-linux"]')
, ('["packages","aarch64-darwin"]')
-- Legacy
, ('["legacyPackages","x86_64-linux"]')
, ('["legacyPackages","x86_64-darwin"]')
, ('["legacyPackages","aarch64-linux"]')
, ('["legacyPackages","aarch64-darwin"]')
-- Catalogs
, ('["catalog","x86_64-linux","stable"]')
, ('["catalog","x86_64-linux","staging"]')
, ('["catalog","x86_64-linux","unstable"]')
, ('["catalog","x86_64-darwin","stable"]')
, ('["catalog","x86_64-darwin","staging"]')
, ('["catalog","x86_64-darwin","unstable"]')
, ('["catalog","aarch64-linux","stable"]')
, ('["catalog","aarch64-linux","staging"]')
, ('["catalog","aarch64-linux","unstable"]')
, ('["catalog","aarch64-darwin","stable"]')
, ('["catalog","aarch64-darwin","staging"]')
, ('["catalog","aarch64-darwin","unstable"]')
;


-- -------------------------------------------------------------------------- --

CREATE VIEW IF NOT EXISTS v_PackageSets_RowNums AS
  SELECT path, row_number() OVER ( ORDER BY path ) AS row_number
  FROM PackageSets;


-- -------------------------------------------------------------------------- --

CREATE VIEW IF NOT EXISTS v_PackageSets_PathParts AS SELECT
    row_number
  , subtree
  , system
  , iif( subtree = 'catalog', json_extract( rest, '$[0]' ), NULL )
    AS stability
  , iif( subtree = 'catalog', json_remove( rest, '$[0]' ), rest ) AS rest
  FROM (
    SELECT   row_number
           , json_extract( path, '$[0]' )        AS subtree
           , json_extract( path, '$[1]' )        AS system
           , json_remove( path, '$[0]', '$[0]' ) AS rest
    FROM v_PackageSets_RowNums
  );


-- -------------------------------------------------------------------------- --

.load ./libsqlexts

CREATE VIEW IF NOT EXISTS v_PackageSets_Hashes AS
  SELECT hash_str( path ) AS hash, path FROM PackageSets;



-- -------------------------------------------------------------------------- --
--
--
--
-- ========================================================================== --
