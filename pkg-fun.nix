# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{ stdenv
, sqlite
, pkg-config
, nlohmann_json
, nix
, boost
, argparse
, semver
}: stdenv.mkDerivation {
  pname   = "flox-resolver";
  version = "0.1.0";
  src     = builtins.path {
    path = ./.;
    filter = name: type: let
      bname   = baseNameOf name;
      ignores = [
        "default.nix"
        "pkg-fun.nix"
        "flake.nix"
        "flake.lock"
        ".ccls"
        ".ccls-cache"
        ".git"
        ".gitignore"
        "out"
        "bin"
        "lib"
      ];
      notIgnored = ! ( builtins.elem bname ignores );
      notObject  = ( builtins.match ".*\\.o" name ) == null;
      notResult  = ( builtins.match "result(-*)?" bname ) == null;
      isSrc      = ( builtins.match ".*\\.cc" name ) != null;
    in notIgnored && notObject && notResult && (
      ( ( dirOf name ) == "tests" ) -> isSrc
    );
  };
  libExt            = stdenv.hostPlatform.extensions.sharedLibrary;
  nativeBuildInputs = [pkg-config];
  buildInputs       = [
    sqlite.dev nlohmann_json nix.dev boost argparse
  ];
  propagatedBuildInputs = [semver];
  makeFlags = [
    "boost_CFLAGS=-I${boost}/include"
    "libExt=${stdenv.hostPlatform.extensions.sharedLibrary}"
    "SEMVER_PATH=${semver}/bin/semver"
  ];
  configurePhase = ''
    runHook preConfigure;
    export PREFIX="$out";
    if [[ "''${enableParallelBuilding:-1}" = 1 ]]; then
      makeFlagsArray+=( '-j4' );
    fi
    runHook postConfigure;
  '';
  # Checks require internet
  doCheck        = false;
  doInstallCheck = false;
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
