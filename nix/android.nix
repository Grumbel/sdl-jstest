# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

# Android APKs of sdl2-jstest and sdl3-jstest, after xanatoo's ndk-build
# pipeline (mk/android/): SDL's native library and Java side are built from
# its source tree (SDL2 release tarball, nixpkgs' SDL3 source), the program
# into libmain.so against it, both packaged into a signed APK.
#
#   nix build .#sdl2-jstest-android
#   nix build .#sdl3-jstest-android
#   nix run .#install-android-sdl3-jstest

{ pkgs
, androidSdk
, buildToolsVersion
, packagePlatform   # minSdk, linked against
, compilePlatform   # the Java side's classpath
, targetAbis
, version
, versionCode
, sdl_gamecontrollerdb
, sdl2Src
, sdl2Version
}:

let
  lib = pkgs.lib;
  abis = lib.concatStringsSep " " targetAbis;

  topAndroidMk = pkgs.writeText "Android.mk" "include $(call all-subdir-makefiles)\n";

  applicationMk = pkgs.writeText "Application.mk" ''
    APP_ABI := ${abis}
    APP_PLATFORM := android-${packagePlatform}
  '';

  sdlLibs = { major, src, sdlVersion }: pkgs.stdenvNoCC.mkDerivation {
    pname = "sdl${toString major}-android-libs";
    version = sdlVersion;
    dontUnpack = true;
    nativeBuildInputs = [ androidSdk pkgs.jdk17 pkgs.gnumake ];
    env = {
      BUILD_TOOLS_VERSION = buildToolsVersion;
      COMPILE_PLATFORM = compilePlatform;
      PACKAGE_PLATFORM = packagePlatform;
      SDL_SRC = "${src}";
      APPLICATION_MK = applicationMk;
      TOP_ANDROID_MK = topAndroidMk;
      TARGET_ABIS = abis;
    };
    buildPhase = ''
      runHook preBuild
      export ANDROID_HOME=${androidSdk}/libexec/android-sdk
      bash ${../mk/android/scripts}/build-sdl-libs.sh
      runHook postBuild
    '';
    dontInstall = true;
    # Android ELF: the host's patchelf and strip must keep off.
    dontPatchELF = true;
    dontStrip = true;
  };

  sdl2Libs = sdlLibs { major = 2; src = sdl2Src; sdlVersion = sdl2Version; };
  sdl3Libs = sdlLibs { major = 3; inherit (pkgs.sdl3) src; sdlVersion = pkgs.sdl3.version; };

  # The sources the APK is built from: no build directories, no .git.
  appSrc = lib.fileset.toSource {
    root = ../.;
    fileset = lib.fileset.unions [ ../src ../data/icons ];
  };

  mkApk = { major, libs }:
    let
      program = "sdl${toString major}-jstest";
      apkName = "${program}-${version}.apk";
      apk = pkgs.stdenvNoCC.mkDerivation {
        pname = "${program}-android";
        inherit version;
        dontUnpack = true;
        nativeBuildInputs = [ androidSdk pkgs.jdk17 pkgs.zip pkgs.gnumake ];
        env = {
          BUILD_TOOLS_VERSION = buildToolsVersion;
          PACKAGE_PLATFORM = packagePlatform;
          TARGET_ABIS = abis;
          PROGRAM = program;
          SDL_MAJOR = toString major;
          # Android ids allow no '-'; otherwise the executable name (INT-1).
          APP_ID = "org.grumbel.sdl${toString major}_jstest";
          APP_LABEL = "SDL${toString major} Joystick Test";
          APP_DIR = "${../mk/android/app}";
          APP_SRC_DIR = "${appSrc}";
          GAMECONTROLLERDB = "${sdl_gamecontrollerdb}/gamecontrollerdb.txt";
          APPLICATION_MK = applicationMk;
          TOP_ANDROID_MK = topAndroidMk;
          SDL_ANDROID_LIBS = libs;
          KEYSTORE = "${../mk/android/keystore/debug.keystore}";
          APP_VERSION = version;
          VERSION_CODE = toString versionCode;
        };
        buildPhase = ''
          runHook preBuild
          export ANDROID_HOME=${androidSdk}/libexec/android-sdk
          bash ${../mk/android/scripts}/build-apk.sh
          runHook postBuild
        '';
        installPhase = ''
          mkdir -p $out
          cp out/${program}.apk $out/${apkName}
        '';
        dontPatchELF = true;
        dontStrip = true;
      };
      installApp = {
        type = "app";
        program = toString (pkgs.writeShellScript "install-${program}-apk" ''
          exec ${pkgs.android-tools}/bin/adb install -r ${apk}/${apkName}
        '');
        meta.description = "Install the ${program} APK on a connected Android device with adb";
      };
    in { inherit apk installApp; };

  sdl2 = mkApk { major = 2; libs = sdl2Libs; };
  sdl3 = mkApk { major = 3; libs = sdl3Libs; };

in {
  inherit sdl2Libs sdl3Libs;
  sdl2Apk = sdl2.apk;
  sdl3Apk = sdl3.apk;
  installSdl2 = sdl2.installApp;
  installSdl3 = sdl3.installApp;
}
