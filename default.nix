# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{ nixpkgs       ? builtins.getFlake "nixpkgs"
, floco         ? builtins.getFlake "github:aakropotkin/floco"
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
}: import ./pkg-fun.nix {
  inherit stdenv sqlite pkg-config nlohmann_json nix boost argparse semver;
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
