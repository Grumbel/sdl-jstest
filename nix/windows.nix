# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

# Windows builds (mingw-w64 cross, win32 and win64). SDL3, SDL2 2.32 and
# sdl12-compat are built here from source: nixpkgs' mingw SDL packages pull
# in X11, which does not cross-build, and SDL needs nothing external on
# Windows. sdl12-compat loads SDL2.dll at run time.
#
# Output (gamepkgstd DIR-2, FLAKE-1/3): the three .exe, their DLLs and
# data/gamecontrollerdb.txt side by side; the zip has one top-level
# directory sdl-jstest-<version>-<target>/.
#
#   nix build .#sdl-jstest-win64
#   nix build .#sdl-jstest-win64-zip

{ pkgs            # native, for tools
, crossPkgs       # pkgsCross.mingwW64 / mingw32
, target          # "win64" / "win32"
, version
, sdl_gamecontrollerdb
, sdl2Src
, sdl2Version
}:

let
  lib = pkgs.lib;
  stdenv = crossPkgs.stdenv;

  sdlFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DSDL_TESTS=OFF"
    "-DSDL_TEST_LIBRARY=OFF"
    "-DSDL_EXAMPLES=OFF"
    "-DSDL_STATIC=OFF"
  ];

  sdl3 = stdenv.mkDerivation {
    pname = "sdl3-${target}";
    inherit (pkgs.sdl3) version src;
    nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
    cmakeFlags = sdlFlags ++ [ "-DSDL_REVISION=SDL-${pkgs.sdl3.version}" ];
  };

  sdl2 = stdenv.mkDerivation {
    pname = "sdl2-${target}";
    version = sdl2Version;
    src = sdl2Src;
    nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
    cmakeFlags = sdlFlags ++ [ "-DSDL_TEST=OFF" ];
    # Its sdl2.pc joins prefix and absolute paths ("//nix/store"), which
    # nixpkgs' fixup rejects; CMake uses SDL2Config.cmake.
    postInstall = "rm -r $out/lib/pkgconfig";
  };

  sdl12compat = stdenv.mkDerivation {
    pname = "sdl12-compat-${target}";
    inherit (pkgs.sdl12-compat) version src;
    nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
    cmakeFlags = [
      "-DCMAKE_BUILD_TYPE=Release"
      "-DSDL12TESTS=OFF"
      "-DCMAKE_DISABLE_FIND_PACKAGE_SDL2=ON"
      "-DSDL2_INCLUDE_DIRS=${sdl2}/include/SDL2"
    ];
  };

  build = stdenv.mkDerivation {
    pname = "sdl-jstest-${target}-build";
    inherit version;
    src = lib.cleanSource ../.;
    nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
    postPatch = ''
      rm -rf external/sdl_gamecontrollerdb
      ln -s "${sdl_gamecontrollerdb}" external/sdl_gamecontrollerdb
    '';
    cmakeFlags = [
      "-DCMAKE_BUILD_TYPE=Release"
      "-DPROJECT_VERSION_FULL=${version}"
      "-DWARNINGS=ON"
      "-DBUILD_TESTS=OFF"
      "-DSDL_JSTEST_NCURSES=OFF"
      "-DSDL_INCLUDE_DIR=${sdl12compat}/include/SDL"
      # No SDLmain (see sdl1-jstest.c).
      "-DSDL_LIBRARY=${sdl12compat}/lib/libSDL.dll.a"
      "-DSDL2_DIR=${sdl2}/lib/cmake/SDL2"
      "-DSDL3_DIR=${sdl3}/lib/cmake/SDL3"
    ];
  };

  dirName = "sdl-jstest-${version}-${target}";

  objdump = "${stdenv.cc.bintools.bintools}/bin/${stdenv.cc.targetPrefix}objdump";

  dist = pkgs.runCommand "sdl-jstest-${version}-${target}" { } ''
    d=$out/${dirName}
    mkdir -p $d/data
    cp ${build}/bin/sdl1-jstest.exe ${build}/bin/sdl2-jstest.exe ${build}/bin/sdl3-jstest.exe $d/
    cp ${sdl12compat}/bin/SDL.dll ${sdl2}/bin/SDL2.dll ${sdl3}/bin/SDL3.dll $d/
    # Runtime DLLs of the mingw toolchain the programs or SDL import
    # (threads, libgcc), found next to the cross compiler.
    for dll in $(${objdump} -p $d/*.exe $d/*.dll 2>/dev/null \
                 | sed -n 's/^\tDLL Name: //p' | sort -u); do
      case "$dll" in
        SDL*.dll|sdl*.dll) ;;
        *)
          f=$(find ${lib.concatStringsSep " " (map toString [ stdenv.cc.cc stdenv.cc.libc crossPkgs.windows.mcfgthreads ])} \
                -iname "$dll" -print -quit 2>/dev/null || true)
          if [ -n "$f" ]; then cp -L "$f" $d/; fi ;;
      esac
    done
    chmod u+w $d/*.dll
    cp ${build}/share/sdl-jstest/gamecontrollerdb.txt $d/data/
    cp ${../LICENSES/GPL-3.0-or-later.txt} $d/LICENSE.txt
    cp ${../README.md} $d/README.md
  '';

  zip = pkgs.runCommand "sdl-jstest-${version}-${target}-zip" {
    nativeBuildInputs = [ pkgs.zip ];
  } ''
    mkdir -p $out
    cd ${dist}
    zip -r -9 $out/${dirName}.zip ${dirName}
  '';

in {
  inherit sdl2 sdl3 sdl12compat build dist zip;
}
