# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{ nixpkgs         ? builtins.getFlake "nixpkgs"
, floco           ? builtins.getFlake "github:aakropotkin/floco"
, sql-builder-src ? builtins.fetchTree {
                    type = "github"; owner = "six-ddc"; repo = "sql-builder";
                  }
, system        ? builtins.currentSystem
, pkgsFor       ? nixpkgs.legacyPackages.${system}
, stdenv        ? pkgsFor.stdenv
, sqlite        ? pkgsFor.sqlite
, pkg-config    ? pkgsFor.pkg-config
, nlohmann_json ? pkgsFor.nlohmann_json
, nix           ? pkgsFor.nix
, boost         ? pkgsFor.boost
, argparse      ? pkgsFor.argparse
, semver        ? floco.legacyPackages.${system}.semver
, sql-builder   ? pkgsFor.runCommandNoCC "sql-builder" {
                    src = sql-builder-src;
                  } ''
                    mkdir -p "$out/include/sql-builder";
                    cp "$src/sql.h" "$out/include/sql-builder/sql.hh";
                  ''
}: import ./pkg-fun.nix {
  inherit
    stdenv sqlite pkg-config nlohmann_json nix boost argparse semver sql-builder
  ;
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
