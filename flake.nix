# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{

# ---------------------------------------------------------------------------- #

  inputs.nixpkgs.url = "github:NixOS/nixpkgs";
  inputs.floco.url   = "github:aakropotkin/floco";
  inputs.sql-builder = {
    url   = "github:six-ddc/sql-builder";
    flake = false;
  };


# ---------------------------------------------------------------------------- #

  outputs = { nixpkgs, floco, sql-builder, ... }: let

# ---------------------------------------------------------------------------- #

    eachDefaultSystemMap = let
      defaultSystems = [
        "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin"
      ];
    in fn: let
      proc = system: { name = system; value = fn system; };
    in builtins.listToAttrs ( map proc defaultSystems );


# ---------------------------------------------------------------------------- #

    overlays.deps         = floco.overlays.default;
    overlays.flox-resolve = final: prev: {
      flox-resolve = final.callPackage ./pkg-fun.nix {};
      sql-builder  = final.runCommandNoCC "sql-builder" {
        src = sql-builder;
      } ''
        mkdir -p "$out/include/sql-builder";
        cp "$src/sql.h" "$out/include/sql-builder/sql.hh";
      '';
    };
    overlays.default = nixpkgs.lib.composeExtensions overlays.deps
                                                     overlays.flox-resolve;


# ---------------------------------------------------------------------------- #

    packages = eachDefaultSystemMap ( system: let
      pkgsFor = ( builtins.getAttr system nixpkgs.legacyPackages ).extend
                  overlays.default;
    in {
      inherit (pkgsFor) flox-resolve sql-builder;
      default = pkgsFor.flox-resolve;
    } );


# ---------------------------------------------------------------------------- #

  in {

    inherit overlays packages;
    legacyPackages = packages;

  };


# ---------------------------------------------------------------------------- #


}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
