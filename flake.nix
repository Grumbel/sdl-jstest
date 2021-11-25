{
  description = "Generate a stream of pseudo random numbers";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    nix.inputs.nixpkgs.follows = "nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nix, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in rec {
        packages = flake-utils.lib.flattenTree {
          sdl-jstest = pkgs.stdenv.mkDerivation {
            pname = "sdl-jstest";
            version = "0.0.0";
            src = nixpkgs.lib.cleanSource ./.;
            nativeBuildInputs = [
              pkgs.cmake
              pkgs.ninja
              pkgs.gcc
              pkgs.pkgconfig
            ];
            buildInputs = [
              pkgs.SDL
              pkgs.SDL
              pkgs.ncurses
            ];
           };
        };
        defaultPackage = packages.sdl-jstest;
      });
}
