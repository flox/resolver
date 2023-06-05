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
}: stdenv.mkDerivation {
  pname   = "flox-resolver";
  version = "0.1.0";
  src     = builtins.path {
    path = ./.;
    filter = name: type: let
      bname   = baseNameOf name;
      ignores = [
        "flox-resolve"
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
    in notIgnored && notObject && notResult;
  };
  libExt            = stdenv.hostPlatform.extensions.sharedLibrary;
  nativeBuildInputs = [pkg-config];
  buildInputs       = [
    sqlite.dev nlohmann_json nix.dev boost
  ];
  makeFlags = [
    "boost_CFLAGS=-I${boost}/include"
    "libExt=${stdenv.hostPlatform.extensions.sharedLibrary}"
  ];
  configurePhase = ''
    runHook preConfigure;
    export PREFIX="$out";
    runHook postConfigure;
  '';
}


# ---------------------------------------------------------------------------- #
#
#
#
# ============================================================================ #
