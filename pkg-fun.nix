# ============================================================================ #
#
#
#
# ---------------------------------------------------------------------------- #

{ stdenv, pkg-config, flox-pkgdb, sql-builder }: stdenv.mkDerivation {
  pname   = "flox-resolver";
  version = builtins.replaceStrings ["\n"] [""] ( builtins.readFile ./version );
  src     = builtins.path {
    path = ./.;
    filter = name: type: let
      bname   = baseNameOf name;
      ignores = [
        "default.nix" "pkg-fun.nix" "flake.nix" "flake.lock"
        ".ccls" ".ccls-cache"
        ".git" ".gitignore"
        "out" "bin" "lib"
        "tests"  # Tests require internet so there's no point in including them
      ];
      ext = let
        m = builtins.match ".*\\.([^.]+)" name;
      in if m == null then "" else builtins.head m;
      ignoredExts = ["o" "so" "dylib"];
      notIgnored  = ( ! ( builtins.elem bname ignores ) ) &&
                    ( ! ( builtins.elem ext ignoredExts ) );
      notResult = ( builtins.match "result(-*)?" bname ) == null;
    in notIgnored && notResult;
  };

  propagatedBuildInputs = flox-pkgdb.propagatedBuildInputs ++ [flox-pkgdb];
  buildInputs           = flox-pkgdb.buildInputs ++ [sql-builder];
  inherit (flox-pkgdb)
    nativeBuildInputs
    nix_INCDIR
    boost_CFLAGS
    libExt
    SEMVER_PATH
  ;
  sql_builder_CFLAGS = "-I" + sql-builder.outPath + "/include";
  configurePhase     = ''
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
