{ stdenv
, lib

, SDL ? null
, SDL2 ? null
, SDL3 ? null
, ncurses
, cmake
, pkg-config

, version
, sdl_gamecontrollerdb

  # Which binaries to build (maps to CMake BUILD_SDL*_JSTEST)
, buildSdl ? true
, buildSdl2 ? true
, buildSdl3 ? true
}:

assert buildSdl || buildSdl2 || buildSdl3;
assert buildSdl -> SDL != null;
assert buildSdl2 -> SDL2 != null;
assert buildSdl3 -> SDL3 != null;

let
  pname =
    if buildSdl && buildSdl2 && buildSdl3 then "sdl-jstest"
    else if buildSdl3 && !buildSdl && !buildSdl2 then "sdl3-jstest"
    else if buildSdl2 && !buildSdl && !buildSdl3 then "sdl2-jstest"
    else if buildSdl && !buildSdl2 && !buildSdl3 then "sdl1-jstest"
    else "sdl-jstest";
in
stdenv.mkDerivation {
  inherit pname version;

  src = ./.;

  # CTest unit tests (gamepkgstd FLAKE-7); skipped automatically when cross.
  doCheck = true;

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
    "-DWARNINGS=ON"
    "-DWERROR=ON"
    "-DPROJECT_VERSION_FULL=${version}"
    "-DBUILD_SDL_JSTEST=${if buildSdl then "ON" else "OFF"}"
    "-DBUILD_SDL2_JSTEST=${if buildSdl2 then "ON" else "OFF"}"
    "-DBUILD_SDL3_JSTEST=${if buildSdl3 then "ON" else "OFF"}"
  ];

  # The newest mappings from the flake input instead of the copy in the tree.
  postPatch = lib.optionalString (buildSdl2 || buildSdl3) ''
    rm -rf external/sdl_gamecontrollerdb
    mkdir -p external/
    ln -s "${sdl_gamecontrollerdb}" external/sdl_gamecontrollerdb
  '';

  nativeBuildInputs = [
    cmake
    pkg-config
  ];

  buildInputs = [
    ncurses
  ]
  ++ lib.optional buildSdl SDL
  ++ lib.optional buildSdl2 SDL2
  ++ lib.optional buildSdl3 SDL3;

  meta = {
    description = "Test joysticks and game controllers through SDL 1.2, SDL2 and SDL3";
    homepage = "https://github.com/grumbel/sdl-jstest";
    license = lib.licenses.gpl3Plus;
    platforms = lib.platforms.unix;
    mainProgram =
      if buildSdl2 then "sdl2-jstest"
      else if buildSdl3 then "sdl3-jstest"
      else "sdl1-jstest";
  };
}
