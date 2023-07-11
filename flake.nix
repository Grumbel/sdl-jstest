{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.05";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    sdl_gamecontrollerdb.url = "github:gabomdq/SDL_GameControllerDB";
    sdl_gamecontrollerdb.flake = false;

    SDL-win32.url = "github:grumnix/SDL-win32";
    SDL-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL-win32.inputs.tinycmmc.follows = "tinycmmc";

    SDL2-win32.url = "github:grumnix/SDL2-win32";
    SDL2-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL2-win32.inputs.tinycmmc.follows = "tinycmmc";
  };

  outputs = { self, nixpkgs, flake-utils, tinycmmc, sdl_gamecontrollerdb, SDL-win32, SDL2-win32 }:
    tinycmmc.lib.eachSystemWithPkgs (pkgs:
      let
        pkgs_mingw32 = nixpkgs.legacyPackages.${pkgs.system}.pkgsCross.mingw32;
      in {
        packages = rec {
          default = sdl-jstest;

          ncurses-win32 = (pkgs.ncurses.override {
            # enableStatic = true;
          }).overrideAttrs (final: prev: {
            preConfigure = ''
              # workaround /homeless-shelter/.wine not writable
              export HOME=$TEMPDIR
            '' + prev.preConfigure;

            preFixup = ''
              # do nothing
            '';
          });

          sdl-jstest = pkgs.callPackage ./sdl-jstest.nix {
            inherit self;
            inherit sdl_gamecontrollerdb;

            # stdenv = pkgs.gcc12Stdenv;
            tinycmmc = tinycmmc.packages.${pkgs.system}.default;

            SDL = if pkgs.targetPlatform.isWindows
                   then SDL-win32.packages.${pkgs.system}.default
                   else pkgs.SDL;

            SDL2 = if pkgs.targetPlatform.isWindows
                   then SDL2-win32.packages.${pkgs.system}.default
                   else pkgs.SDL2;

            ncurses = if pkgs.targetPlatform.isWindows
                      then ncurses-win32
                      else pkgs.ncurses;

            mcfgthreads = if pkgs.targetPlatform.isWindows
                          then pkgs.windows.mcfgthreads
                          else null;
          };

          sdl-jstest-win32 = pkgs.runCommand "sdl-jstest-win32" {} ''
            mkdir -p $out
            mkdir -p $out/data/

            cp -vr ${sdl-jstest}/bin/*.exe $out/
            cp -vLr ${sdl-jstest}/bin/*.dll $out/
            cp -vr ${sdl-jstest}/share/sdl-jstest/. $out/data/
          '';

          sdl-jstest-win32-zip = pkgs.runCommand "sdl-jstest-win32-zip" {} ''
            mkdir -p $out
            WORKDIR=$(mktemp -d)

            cp --no-preserve mode,ownership --verbose --recursive \
              ${sdl-jstest-win32}/. "$WORKDIR"

            cd "$WORKDIR"
            ${nixpkgs.legacyPackages.x86_64-linux.zip}/bin/zip \
              -r \
              $out/sdl-jstest-${sdl-jstest.version}-${pkgs.system}.zip \
              .
          '';
        };
      }
    );
}
