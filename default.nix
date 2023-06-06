# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{ nixpkgs       ? builtins.getFlake "nixpkgs"
, system        ? builtins.currentSystem
, pkgsFor       ? nixpkgs.legacyPackages.${system}
, stdenv        ? pkgsFor.stdenv
, sqlite        ? pkgsFor.sqlite
, pkg-config    ? pkgsFor.pkg-config
, nlohmann_json ? pkgsFor.nlohmann_json
, nix           ? pkgsFor.nix
, boost         ? pkgsFor.boost
}: import ./pkg-fun.nix {
  inherit stdenv sqlite pkg-config nlohmann_json nix boost;
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #