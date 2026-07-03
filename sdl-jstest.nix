{ self
, stdenv
, lib

, SDL
, SDL2
, ncurses
, cmake
, pkg-config
, appstream-glib
, mcfgthreads

, tinycmmc
, sdl_gamecontrollerdb
}:

let
  versionBase = lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
  gitRev = "${self.shortRev or self.dirtyShortRev or "dirty"}";
  version = "${versionBase}+g${gitRev}";
in
stdenv.mkDerivation {
  pname = "sdl-jstest";
  inherit version;

  src = ./.;

  doCheck = true;

  cmakeFlags = [
    "-DWARNINGS=ON"
    "-DWERROR=ON"
    "-DBUILD_TESTS=ON"
  ];

  patchPhase = ''
    # Nix behaves differently on a dirty git repository than on a clean one,
    # so we have to check.
    if [ -d external/sdl_gamecontrollerdb ]; then
      rmdir external/sdl_gamecontrollerdb
    fi
    mkdir -p external/
    ln -s "${sdl_gamecontrollerdb}" external/sdl_gamecontrollerdb
  '';

  postFixup = ''
  '' + (lib.optionalString stdenv.hostPlatform.isWindows ''
    mkdir -p $out/bin/
    find ${stdenv.cc.cc} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
    find ${mcfgthreads} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
    ln -sfv ${SDL2}/bin/*.dll $out/bin/
    ln -sfv ${SDL}/bin/*.dll $out/bin/
    ln -sfv ${ncurses.dev}/bin/*.dll $out/bin/
  '');

  nativeBuildInputs = [
    cmake
    pkg-config
  ];

  buildInputs = [
    SDL
    SDL2
    ncurses

    tinycmmc
  ];
}
