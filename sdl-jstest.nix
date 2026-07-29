{ self
, stdenv
, lib

, SDL ? null
, SDL2 ? null
, SDL3 ? null
, ncurses
, cmake
, pkg-config
, appstream-glib
, mcfgthreads ? null

, tinycmmc
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
  versionBase = lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
  gitRev = "${self.shortRev or self.dirtyShortRev or "dirty"}";
  version = "${versionBase}+g${gitRev}";

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

  doCheck = true;

  cmakeFlags = [
    "-DWARNINGS=ON"
    "-DWERROR=ON"
    "-DPROJECT_VERSION_FULL=${version}"
    "-DBUILD_SDL_JSTEST=${if buildSdl then "ON" else "OFF"}"
    "-DBUILD_SDL2_JSTEST=${if buildSdl2 then "ON" else "OFF"}"
    "-DBUILD_SDL3_JSTEST=${if buildSdl3 then "ON" else "OFF"}"
  ];

  patchPhase = lib.optionalString (buildSdl2 || buildSdl3) ''
    # Nix behaves differently on a dirty git repository than on a clean one,
    # so we have to check.
    if [ -d external/sdl_gamecontrollerdb ]; then
      rmdir external/sdl_gamecontrollerdb
    fi
    mkdir -p external/
    ln -s "${sdl_gamecontrollerdb}" external/sdl_gamecontrollerdb
  '';

  postFixup = lib.optionalString stdenv.hostPlatform.isWindows ''
    mkdir -p $out/bin/
    find ${stdenv.cc.cc} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
    ${lib.optionalString (mcfgthreads != null) ''
      find ${mcfgthreads} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
    ''}
    ${lib.optionalString buildSdl3 ''
      ln -sfv ${SDL3}/bin/*.dll $out/bin/ || true
    ''}
    ${lib.optionalString buildSdl2 ''
      ln -sfv ${SDL2}/bin/*.dll $out/bin/
    ''}
    ${lib.optionalString buildSdl ''
      ln -sfv ${SDL}/bin/*.dll $out/bin/
    ''}
    ln -sfv ${ncurses.dev}/bin/*.dll $out/bin/
  '';

  nativeBuildInputs = [
    cmake
    pkg-config
  ];

  buildInputs = [
    ncurses
    tinycmmc
  ]
  ++ lib.optional buildSdl SDL
  ++ lib.optional buildSdl2 SDL2
  ++ lib.optional buildSdl3 SDL3;
}
