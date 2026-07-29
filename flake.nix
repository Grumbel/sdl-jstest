{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
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
        SDL = if pkgs.stdenv.hostPlatform.isWindows
              then SDL-win32.packages.${pkgs.stdenv.hostPlatform.system}.default
              else pkgs.SDL;

        SDL2 = if pkgs.stdenv.hostPlatform.isWindows
               then SDL2-win32.packages.${pkgs.stdenv.hostPlatform.system}.default
               else pkgs.SDL2;

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

        ncurses = if pkgs.stdenv.hostPlatform.isWindows
                  then ncurses-win32
                  else pkgs.ncurses;

        mcfgthreads = if pkgs.stdenv.hostPlatform.isWindows
                      then pkgs.windows.mcfgthreads
                      else null;

        callSdlJstest = args: pkgs.callPackage ./sdl-jstest.nix ({
          inherit self sdl_gamecontrollerdb SDL SDL2 ncurses mcfgthreads;
        } // args);

        # Full package: both SDL1 and SDL2 binaries
        sdl-jstest = callSdlJstest {
          buildSdl = true;
          buildSdl2 = true;
        };

        # SDL 1.2 only
        sdl1-jstest = callSdlJstest {
          buildSdl = true;
          buildSdl2 = false;
        };

        # SDL 2 only
        sdl2-jstest = callSdlJstest {
          buildSdl = false;
          buildSdl2 = true;
        };
      in
      {
        packages = rec {
          default = sdl-jstest;

          inherit ncurses-win32 sdl-jstest sdl1-jstest sdl2-jstest;

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
              $out/sdl-jstest-${sdl-jstest.version}-${pkgs.stdenv.hostPlatform.system}.zip \
              .
          '';
        };

        apps = {
          default = {
            type = "app";
            program = "${sdl2-jstest}/bin/sdl2-jstest";
          };

          sdl1-jstest = {
            type = "app";
            program = "${sdl1-jstest}/bin/sdl-jstest";
          };

          sdl2-jstest = {
            type = "app";
            program = "${sdl2-jstest}/bin/sdl2-jstest";
          };
        };
      }
    );
}
