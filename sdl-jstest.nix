{ self
, stdenv
, lib

, SDL
, SDL2
, ncurses
, cmake
, pkgconfig
, mcfgthreads

, tinycmmc
, sdl_gamecontrollerdb
}:

stdenv.mkDerivation {
  pname = "sdl-jstest";
  version = "0.2.2";

  src = ./.;

  cmakeFlags = [
    "-DWARNINGS=ON"
    "-DWERROR=ON"
  ];

  patchPhase = ''
    # Nix behaves differently on a dirty git repository than on a clean one,
    # so we have to check.
    if [ -d external/sdl_gamecontrollerdb ]; then
      rmdir external/sdl_gamecontrollerdb
    fi
    ln -s "${sdl_gamecontrollerdb}" external/sdl_gamecontrollerdb
  '';

  postFixup = ''
  '' + (lib.optionalString stdenv.targetPlatform.isWindows ''
    mkdir -p $out/bin/
    find ${stdenv.cc.cc} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
    find ${mcfgthreads} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
    ln -sfv ${SDL2}/bin/*.dll $out/bin/
    ln -sfv ${SDL}/bin/*.dll $out/bin/
    ln -sfv ${ncurses.dev}/bin/*.dll $out/bin/
  '');

  nativeBuildInputs = [
    cmake
    pkgconfig
  ];

  buildInputs = [
    SDL
    SDL2
    ncurses

    tinycmmc
  ];
}
