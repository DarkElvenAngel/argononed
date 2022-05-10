{
  lib, stdenv,
  nix-gitignore,
  dtc, installShellFiles,
  logLevel ? 5,
  ...
}:
let
  cleanSrc = let
    repo = ../..;
    ignores = ''
      # Negate the repo root
      /*

      # Then take what we need
      !/version
      !/makefile
      !/configure
      !/src
      !/OS

      # Not other OS'es
      /OS/*
      !/OS/_common/
      !/OS/nixos/
    '';
  in builtins.path {
    path = repo;
    name = "argononed-src";
    filter = nix-gitignore.gitignoreFilterPure (_: _: true) ignores repo;
  };
in stdenv.mkDerivation {
  pname = "argononed";
  version = lib.strings.fileContents (cleanSrc + "/version");

  src = cleanSrc;

  nativeBuildInputs = [
    dtc # Compilation
    installShellFiles # Completions
  ];

  preConfigure = ''
    patchShebangs --build ./configure
    export TARGET_DISTRO=nixos
  '';

  patches = "OS/nixos/patches/*.patch";

  buildFlags = [ "LOGLEVEL=${toString logLevel}" ];

  installFlags = [ "NIX_DRVOUT=$(out)" ];

  postInstall = ''
    installShellCompletion --bash --name argonone-cli OS/_common/argonone-cli-complete.bash
  '';

  meta = {
    description = "A replacement daemon for the Argon One Raspberry Pi case";
    homepage = "https://gitlab.com/DarkElvenAngel/argononed";
    license = lib.licenses.mit;
    platforms = [ "aarch64-linux" ];
  };
}

