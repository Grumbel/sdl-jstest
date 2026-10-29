# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

# The R36S handheld (RK3326, 640x480) running ArkOS, whose userland is
# Ubuntu 19.10 (eoan): glibc 2.30, SDL2 2.0.x (KMS/DRM). A binary built
# against nixpkgs' glibc does not run there, so everything is compiled with
# nixpkgs' aarch64 cross GCC against a sysroot made of eoan's arm64
# packages (Ubuntu archive, published hashes); same approach as xanatoo's
# and kurvenrausch's nix/r36s.nix.
#
# Per program:
#   sdl2-jstest  links the device's libSDL2-2.0.so.0 (through a link stub
#                with its soname); the launcher preloads the newest SDL2
#                ArkOS has installed.
#   sdl1-jstest  links sdl12-compat, built here and shipped in libs/: real
#                SDL 1.2 has no KMS/DRM driver, sdl12-compat runs on the
#                device's SDL2.
#   sdl3-jstest  links SDL3, built here for KMS/DRM (libdrm / gbm / EGL /
#                udev loaded at runtime from the device) and shipped in libs/.
#
#   nix build .#sdl-jstest-r36s
#   nix build .#sdl-jstest-r36s-portmaster
#   nix build .#sdl-jstest-r36s-portmaster-zip

{ pkgs
, version
, sdl_gamecontrollerdb
}:

let
  lib = pkgs.lib;

  # eoan arm64 packages: glibc, kernel headers, GCC 9 runtime, SDL2 2.0.10,
  # ncurses (terminal UI over ssh), libdrm / gbm / udev headers for SDL3.
  debs = [
    { file = "pool/main/g/glibc/libc6_2.30-0ubuntu2_arm64.deb"; sha256 = "4853da1e3c9eae109708b30dc5df00b7ced888c09f0e77bf6731015a97fb859a"; }
    { file = "pool/main/g/glibc/libc6-dev_2.30-0ubuntu2_arm64.deb"; sha256 = "a0b1860c16c50934a5fe3206ac386267d24b015b0ab677005d38e60ac1d27823"; }
    { file = "pool/main/l/linux/linux-libc-dev_5.3.0-18.19_arm64.deb"; sha256 = "7575a37b1abf722f950fa8202c49f24b9c887dcf8e1f97bad15b555cb8c483de"; }
    { file = "pool/main/g/gcc-9/libgcc1_9.2.1-9ubuntu2_arm64.deb"; sha256 = "81dfdee2c50f13dab98967b0062e51f1cebb967f407e3593c5a77ed2146ea008"; }
    { file = "pool/main/g/gcc-9/libgcc-9-dev_9.2.1-9ubuntu2_arm64.deb"; sha256 = "4e4a83c4d83471eff726fd819b4e5d266fd01c0fbb66f1376718b78bfd4ae116"; }
    { file = "pool/universe/libs/libsdl2/libsdl2-2.0-0_2.0.10+dfsg1-1ubuntu1_arm64.deb"; sha256 = "34f06694f13bbee317b89be868768e8fd5b030287431a630881f45ff99dc9264"; }
    { file = "pool/universe/libs/libsdl2/libsdl2-dev_2.0.10+dfsg1-1ubuntu1_arm64.deb"; sha256 = "9eb97797c392b68449045d814f8763ca984853abefff94ababdc72270b7d65ea"; }
    # Headers only: eoan's SDL_config.h enables X11, so SDL_syswm.h includes
    # Xlib.h (sdl12-compat includes it). Nothing links libX11.
    { file = "pool/main/libx/libx11/libx11-dev_1.6.8-1_arm64.deb"; sha256 = "f15442eb2f001a27f88296c8646493bb8fb2636b59fdd2093c30b4bfe9b4c455"; }
    { file = "pool/main/x/xorgproto/x11proto-dev_2018.4-4_all.deb"; sha256 = "1797fb12c67d037fb530e7203bf38a2c94e1e239bf654432506181c9d43bda01"; }
    { file = "pool/main/n/ncurses/libtinfo6_6.1+20190803-1ubuntu1_arm64.deb"; sha256 = "400f287d1f4d378ca330bbce31e7f6463e8994697b0906ada7dc0cf3dcfbde9f"; }
    { file = "pool/main/n/ncurses/libncurses6_6.1+20190803-1ubuntu1_arm64.deb"; sha256 = "dc1b70da2732e9023cf423f7cd97695628e7ec4b6e3a19cf137274dad59c98ac"; }
    { file = "pool/main/n/ncurses/libncursesw6_6.1+20190803-1ubuntu1_arm64.deb"; sha256 = "c04dc194e629496c41ada96609c5aeb2341968a4712d6b6092dd5666e1c9f1ae"; }
    { file = "pool/main/n/ncurses/libncurses-dev_6.1+20190803-1ubuntu1_arm64.deb"; sha256 = "abbc307e4f0929275d049bc6efab4e019938a93d7dd8edd26972957114810ade"; }
    { file = "pool/main/libd/libdrm/libdrm2_2.4.99-1ubuntu1_arm64.deb"; sha256 = "570407f42d8fac13c20be388fa45a1739ae0e900b30d9a924bd6599cd5e64d45"; }
    { file = "pool/main/libd/libdrm/libdrm-dev_2.4.99-1ubuntu1_arm64.deb"; sha256 = "69071f9360391bdf3ec9a3842aef851b05aac363e9c35b89a8c31f8227d31af5"; }
    { file = "pool/main/m/mesa/libgbm1_19.2.1-1ubuntu1_arm64.deb"; sha256 = "c982ba0bc2e00522e82bdde0b510fee50185b89670ff77853074d7b591c1f054"; }
    { file = "pool/main/m/mesa/libgbm-dev_19.2.1-1ubuntu1_arm64.deb"; sha256 = "7732f9ebe793b7ea5dd45b7c7c531446ea6d9a4f366229630e70a1bd9a58252a"; }
    { file = "pool/main/s/systemd/libudev1_242-7ubuntu3_arm64.deb"; sha256 = "af9cb069b6900a330d6ccf82f9e346ab1d74d5d8df76f4a50b76f63602904610"; }
    { file = "pool/main/s/systemd/libudev-dev_242-7ubuntu3_arm64.deb"; sha256 = "2785dd52d59b7a590f5471b474fd8f09fd2ef5c79d889510b740e5ce95fc69fa"; }
  ];
  fetchDeb = d: pkgs.fetchurl {
    url = "http://old-releases.ubuntu.com/ubuntu/${d.file}";
    inherit (d) sha256;
  };

  sysroot = pkgs.stdenvNoCC.mkDerivation {
    pname = "arkos-sysroot-sdl-jstest";
    version = "eoan-arm64";
    dontUnpack = true;
    nativeBuildInputs = [ pkgs.dpkg ];
    installPhase = ''
      mkdir -p $out
      ${lib.concatMapStrings (d: "dpkg-deb -x ${fetchDeb d} $out\n") debs}
      find $out -type l | while read -r link; do
        target=$(readlink "$link")
        case "$target" in
          /*) ln -sfn "$out$target" "$link" ;;
        esac
      done
      # nixpkgs' GCC links -lgcc_s_asneeded.
      echo 'INPUT ( AS_NEEDED ( -lgcc_s ) )' > $out/usr/lib/gcc/${multiarch}/9/libgcc_s_asneeded.so
    '';
    dontFixup = true;
  };

  cross = pkgs.pkgsCross.aarch64-multiplatform.stdenv.cc;
  gcc = cross.cc;
  binutils = cross.bintools.bintools;
  tp = lib.removeSuffix "-" cross.targetPrefix;
  multiarch = "aarch64-linux-gnu";
  gccLib = "${gcc}/lib/gcc/${tp}/${gcc.version}";

  cflags = lib.concatStringsSep " " [
    "-nostdinc"
    "--sysroot=${sysroot}"
    "-B${binutils}/${tp}/bin"
    "-isystem ${gccLib}/include"
    "-isystem ${sysroot}/usr/include/${multiarch}"
    "-isystem ${sysroot}/usr/include"
    "-march=armv8-a -mtune=cortex-a35"
    "-mno-outline-atomics"
  ];
  ldflags = lib.concatStringsSep " " [
    "-B${sysroot}/usr/lib/${multiarch}"
    "-B${gccLib}"
    "-L${sysroot}/usr/lib/gcc/${multiarch}/9"
    "-L${sysroot}/usr/lib/${multiarch}"
    "-L${sysroot}/lib/${multiarch}"
    "-Wl,-rpath-link,${sysroot}/usr/lib/${multiarch}:${sysroot}/lib/${multiarch}"
    "-Wl,--dynamic-linker=/lib/ld-linux-aarch64.so.1"
  ];
  cc = pkgs.writeShellScript "arkos-gcc" ''
    for a in "$@"; do
      case "$a" in
        -c|-S|-E|-M|-MM) exec ${gcc}/bin/${tp}-gcc ${cflags} "$@" ;;
      esac
    done
    exec ${gcc}/bin/${tp}-gcc ${cflags} ${ldflags} "$@" -lm
  '';

  crossFlags = [
    "-DCMAKE_SYSTEM_NAME=Linux"
    "-DCMAKE_SYSTEM_PROCESSOR=aarch64"
    "-DCMAKE_SYSROOT=${sysroot}"
    "-DCMAKE_C_COMPILER=${cc}"
    "-DCMAKE_AR=${binutils}/bin/${tp}-ar"
    "-DCMAKE_RANLIB=${binutils}/bin/${tp}-ranlib"
    "-DCMAKE_STRIP=${binutils}/bin/${tp}-strip"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_SKIP_RPATH=ON"
  ];
  crossAttrs = {
    nativeBuildInputs = [ pkgs.cmake pkgs.pkg-config ];
    PKG_CONFIG_PATH = "${sysroot}/usr/lib/${multiarch}/pkgconfig:${sysroot}/usr/share/pkgconfig";
    PKG_CONFIG_LIBDIR = "${sysroot}/usr/lib/${multiarch}/pkgconfig:${sysroot}/usr/share/pkgconfig";
    PKG_CONFIG_SYSROOT_DIR = "${sysroot}";
    dontPatchELF = true;
    dontPatchShebangs = true;
    dontStrip = true;
  };

  # sdl2-jstest needs the SDL >= 2.0.18 API (sensors, touchpads, ...), eoan
  # has 2.0.10 headers: it compiles against nixpkgs' sdl2-compat headers
  # (SDL 2.32 API, LP64 like aarch64) and links a stub with the device's
  # soname and every SDL2 symbol. The launcher preloads the newest SDL2
  # ArkOS has; sdl2-jstest refuses anything older than 2.0.18.
  sdl2Headers = "${lib.getDev pkgs.sdl2-compat}/include/SDL2";
  sdl2Stub = pkgs.runCommand "libSDL2-link-stub" { } ''
    mkdir -p $out/lib
    ${pkgs.binutils}/bin/nm -D --defined-only ${lib.getLib pkgs.sdl2-compat}/lib/libSDL2-2.0.so.0 \
      | awk '$2 == "T" { print "void " $3 "(void) {}" }' > stub.c
    ${cc} -shared -fPIC -o $out/lib/libSDL2.so -Wl,-soname,libSDL2-2.0.so.0 stub.c
  '';

  # SDL 1.2 API on top of the device's SDL2 (loaded with dlopen, so any
  # 2.0.x works at runtime); it needs newer headers than eoan's, too.
  sdl12compat = pkgs.stdenv.mkDerivation (crossAttrs // {
    pname = "sdl12-compat-r36s";
    inherit (pkgs.sdl12-compat) version src;
    cmakeFlags = crossFlags ++ [
      "-DCMAKE_DISABLE_FIND_PACKAGE_SDL2=ON"
      "-DSDL2_INCLUDE_DIRS=${sdl2Headers}"
      "-DSDL12TESTS=OFF"
    ];
  });

  # SDL3 for KMS/DRM; everything device-specific is loaded at runtime.
  sdl3 = pkgs.stdenv.mkDerivation (crossAttrs // {
    pname = "sdl3-r36s";
    inherit (pkgs.sdl3) version src;
    cmakeFlags = crossFlags ++ [
      "-DSDL_SHARED=ON"
      "-DSDL_STATIC=OFF"
      "-DSDL_TESTS=OFF"
      "-DSDL_EXAMPLES=OFF"
      "-DSDL_REVISION=SDL-${pkgs.sdl3.version}"
      "-DSDL_VENDOR_INFO=sdl-jstest R36S"
      "-DSDL_UNIX_CONSOLE_BUILD=ON"
      "-DSDL_KMSDRM=ON"
      "-DSDL_OPENGLES=ON"
      "-DSDL_OPENGL=OFF"
      "-DSDL_VULKAN=OFF"
      "-DSDL_RENDER_VULKAN=OFF"
      "-DSDL_RENDER_GPU=OFF"
      "-DSDL_X11=OFF"
      "-DSDL_WAYLAND=OFF"
      "-DSDL_ALSA=OFF"
      "-DSDL_PULSEAUDIO=OFF"
      "-DSDL_PIPEWIRE=OFF"
      "-DSDL_JACK=OFF"
      "-DSDL_SNDIO=OFF"
      "-DSDL_DBUS=OFF"
      "-DSDL_IBUS=OFF"
      "-DSDL_LIBURING=OFF"
      "-DSDL_HIDAPI_LIBUSB=OFF"
      "-DSDL_LIBUDEV=ON"
      # eoan has libudev.so under /lib (rerooted into the sysroot).
      "-DCMAKE_LIBRARY_PATH=/lib/${multiarch}"
    ];
  });

  r36s = pkgs.stdenv.mkDerivation (crossAttrs // {
    pname = "sdl-jstest-r36s";
    inherit version;
    src = lib.cleanSource ../.;
    postPatch = ''
      rm -rf external/sdl_gamecontrollerdb
      ln -s "${sdl_gamecontrollerdb}" external/sdl_gamecontrollerdb
    '';
    cmakeFlags = crossFlags ++ [
      "-DPROJECT_VERSION_FULL=${version}"
      "-DWARNINGS=ON"
      "-DBUILD_TESTS=OFF"
      "-DSDL_INCLUDE_DIR=${sdl12compat}/include/SDL"
      "-DSDL_LIBRARY=${sdl12compat}/lib/libSDL-1.2.so"
      "-DSDL2_INCLUDE_DIRS=${sdl2Headers}"
      "-DSDL2_LIBRARIES=${sdl2Stub}/lib/libSDL2.so"
      "-DSDL3_DIR=${sdl3}/lib/cmake/SDL3"
    ];
    postInstall = ''
      for p in sdl1-jstest sdl2-jstest sdl3-jstest; do
        ${binutils}/bin/${tp}-strip $out/bin/$p
      done
    '';
  });

  portmaster = pkgs.stdenvNoCC.mkDerivation {
    pname = "sdl-jstest-r36s-portmaster";
    inherit version;
    dontUnpack = true;
    dontPatchShebangs = true;
    dontFixup = true;
    installPhase = ''
      d=$out/sdl-jstest
      mkdir -p $d/libs
      for p in sdl1-jstest sdl2-jstest sdl3-jstest; do
        install -m755 ${r36s}/bin/$p $d/$p
      done
      cp ${r36s}/share/sdl-jstest/gamecontrollerdb.txt $d/
      cp -L ${sdl12compat}/lib/libSDL-1.2.so.0 $d/libs/
      cp -L ${sdl3}/lib/libSDL3.so.0 $d/libs/
      chmod 755 $d/libs/*.so.0
      ${binutils}/bin/${tp}-strip $d/libs/*.so.0
      install -m755 ${../mk/r36s/launch.sh} $d/launch.sh
      cp ${../LICENSES/GPL-3.0-or-later.txt} $d/LICENSE.txt
      cp ${../data/icons/256/sdl2-jstest.png} $d/cover.png
      cp ${../doc/screenshots/screenshot-0.3.0.png} $out/screenshot.png
      for n in 1 2 3; do
        install -m755 "${../mk/r36s/port.sh}" "$out/SDL$n Joystick Test.sh"
        sed -i "s/@PROGRAM@/sdl$n-jstest/" "$out/SDL$n Joystick Test.sh"
      done
      substitute ${../mk/r36s/port.json} $out/port.json --subst-var-by version ${version}
      cp ${../mk/r36s/README.md} $out/README.md
    '';
  };

  portmasterZip = pkgs.runCommand "sdl-jstest-${version}-r36s-portmaster.zip" {
    nativeBuildInputs = [ pkgs.zip ];
  } ''
    mkdir -p $out
    cd ${portmaster}
    zip -r -9 $out/sdl-jstest.zip .
  '';

in {
  inherit sysroot sdl12compat sdl3 r36s portmaster portmasterZip;
}
