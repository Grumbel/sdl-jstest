# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

# The WebAssembly build (gamepkgstd EMSCRIPTEN.md): sdl2-jstest and
# sdl3-jstest as web pages, after dualthrust's nix/wasm.nix. SDL2 (release
# tarball) and SDL3 (nixpkgs' source) are built here as static wasm32
# libraries instead of Emscripten's ports, which download at build time.
# sdl1-jstest is left out: sdl12-compat loads SDL2 with dlopen().
#
# Output: index.html (picks a program), sdl2-jstest/ and sdl3-jstest/, each
# index.html, <prog>.js, <prog>.wasm and <prog>.data (gamecontrollerdb.txt).
#
#   nix build .#sdl-jstest-wasm   the site, in result/
#   nix run .#sdl-jstest-wasm     serve it locally and open a browser

{ pkgs
, version
, gitRev
, sdl_gamecontrollerdb
, sdl2Src
, sdl2Version
}:

let
  lib = pkgs.lib;
  emscripten = pkgs.emscripten;

  # Emscripten writes to its cache; start from the prebuilt one in the
  # store (libc ...) instead of building those again.
  setupEmCache = name: ''
    export EM_CACHE="$TMPDIR/emcache-${name}"
    cp -r ${emscripten}/share/emscripten/cache "$EM_CACHE"
    chmod -R u+w "$EM_CACHE"
  '';

  sdlWasm = { major, src, sdlVersion }: pkgs.stdenv.mkDerivation {
    pname = "sdl${toString major}-wasm";
    version = sdlVersion;
    dontUnpack = true;
    dontConfigure = true;
    dontUseCmakeConfigure = true;
    nativeBuildInputs = [ emscripten pkgs.cmake pkgs.python3 ];
    env = {
      SDL_SRC = "${src}";
      SDL_MAJOR = toString major;
      SDL_REVISION = "SDL-${sdlVersion}";
    };
    buildPhase = ''
      runHook preBuild
      ${setupEmCache "sdl${toString major}"}
      PREFIX=$PWD/prefix bash ${../mk/wasm/scripts/build-sdl.sh}
      runHook postBuild
    '';
    installPhase = ''
      runHook preInstall
      mkdir -p $out
      cp -a prefix/. $out/
      runHook postInstall
    '';
    dontStrip = true; # the host's strip does not know wasm objects
    meta = {
      description = "Static SDL${toString major} for wasm32-emscripten";
      license = lib.licenses.zlib;
      platforms = lib.platforms.linux;
    };
  };

  sdl2Wasm = sdlWasm { major = 2; src = sdl2Src; sdlVersion = sdl2Version; };
  sdl3Wasm = sdlWasm { major = 3; inherit (pkgs.sdl3) src; sdlVersion = pkgs.sdl3.version; };

  site = pkgs.stdenv.mkDerivation {
    pname = "sdl-jstest-wasm";
    inherit version;
    src = lib.cleanSource ../.;
    dontConfigure = true;
    dontUseCmakeConfigure = true;
    nativeBuildInputs = [ emscripten pkgs.cmake pkgs.python3 ];
    postPatch = ''
      rm -rf external/sdl_gamecontrollerdb
      ln -s "${sdl_gamecontrollerdb}" external/sdl_gamecontrollerdb
    '';
    buildPhase = ''
      runHook preBuild
      ${setupEmCache "sdl-jstest"}
      emcmake cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$out \
        -DPROJECT_VERSION_FULL=${version} \
        -DSDL_JSTEST_GIT_REV=${gitRev} \
        -DWARNINGS=ON \
        -DBUILD_TESTS=OFF \
        -DBUILD_SDL_JSTEST=OFF \
        -DSDL_JSTEST_NCURSES=OFF \
        -DSDL2_INCLUDE_DIRS=${sdl2Wasm}/include/SDL2 \
        -DSDL2_LIBRARIES=${sdl2Wasm}/lib/libSDL2.a \
        -DSDL3_DIR=${sdl3Wasm}/lib/cmake/SDL3
      cmake --build build --parallel ''${NIX_BUILD_CORES:-1}
      runHook postBuild
    '';
    installPhase = ''
      runHook preInstall
      cmake --install build
      runHook postInstall
    '';
    dontStrip = true;
    meta = {
      description = "sdl2-jstest and sdl3-jstest as web pages (WebAssembly)";
      license = lib.licenses.gpl3Plus;
      platforms = lib.platforms.linux;
    };
  };

  # `nix run`: serve the site on 127.0.0.1 and open it in a browser.
  serveApp = {
    type = "app";
    program = toString (pkgs.writeShellScript "sdl-jstest-wasm-serve" ''
      export PKG=${site}
      export APP_NAME=sdl-jstest
      export PATH=${pkgs.python3}/bin:${pkgs.xdg-utils}/bin:$PATH
      exec bash ${../mk/wasm/scripts/serve.sh} "$@"
    '');
    meta.description = "Serve the WebAssembly build of sdl-jstest and open it in a browser";
  };

in {
  inherit sdl2Wasm sdl3Wasm site serveApp;
}
