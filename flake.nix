{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.05";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    sdl_gamecontrollerdb.url = "github:gabomdq/SDL_GameControllerDB";
    sdl_gamecontrollerdb.flake = false;
  };

  outputs = { self, nixpkgs, flake-utils, tinycmmc, sdl_gamecontrollerdb }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        pkgs_mingw32 = nixpkgs.legacyPackages.${system}.pkgsCross.mingw32;
      in {
        packages = ( rec {
          default = sdl-jstest;

          sdl-jstest = pkgs.stdenv.mkDerivation {
            pname = "sdl-jstest";
            version = "0.0.0";
            src = nixpkgs.lib.cleanSource ./.;
            patchPhase = ''
              substituteInPlace CMakeLists.txt \
                 --replace SDL_GameControllerDB/gamecontrollerdb.txt '${sdl_gamecontrollerdb}/gamecontrollerdb.txt'
            '';
            nativeBuildInputs = [
              pkgs.cmake
              pkgs.pkgconfig
            ];
            buildInputs = [
              tinycmmc.packages.${system}.default

              pkgs.SDL
              pkgs.SDL2
              pkgs.ncurses
            ];
           };
        }) // {
          i686-w64-mingw32-sdl-jstest = pkgs.stdenv.mkDerivation {
            pname = "sdl-jstest";
            version = "0.0.0";
            src = nixpkgs.lib.cleanSource ./.;
            patchPhase = ''
                substituteInPlace CMakeLists.txt \
                  --replace SDL_GameControllerDB/gamecontrollerdb.txt '${sdl_gamecontrollerdb}/gamecontrollerdb.txt'
            '';
            nativeBuildInputs = [
              pkgs.cmake
              pkgs.pkgconfig
            ];
            buildInputs = [
              tinycmmc.packages.${system}.default

              # pkgs_mingw32.SDL
              pkgs_mingw32.SDL2
              pkgs_mingw32.ncurses
            ];
          };
        };
      }
    );
}
