#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds the APK of sdl2-jstest or sdl3-jstest: the program as libmain.so
# with ndk-build against the prebuilt SDL, packaged with SDL's Java side,
# gamecontrollerdb.txt as an asset (SDL_RWFromFile reads assets by plain
# name), launcher icons from data/icons/; aligned and signed.
#
# Env: ANDROID_HOME, BUILD_TOOLS_VERSION, PACKAGE_PLATFORM, TARGET_ABIS,
#      PROGRAM (sdl2-jstest / sdl3-jstest), SDL_MAJOR (2 / 3), APP_ID,
#      APP_LABEL, APP_DIR (AndroidManifest.xml, jni/Android.mk), APP_SRC_DIR
#      (the repository: src/, data/icons/), GAMECONTROLLERDB,
#      APPLICATION_MK, TOP_ANDROID_MK, SDL_ANDROID_LIBS, KEYSTORE,
#      APP_VERSION, VERSION_CODE
set -euo pipefail

# shellcheck source=ndk.sh
. "$(dirname "$0")/ndk.sh"
NDK="$(resolve_ndk)"
echo "==> NDK=$NDK"
BT="$ANDROID_HOME/build-tools/$BUILD_TOOLS_VERSION"
PACKAGE_JAR="$ANDROID_HOME/platforms/android-$PACKAGE_PLATFORM/android.jar"

mkdir -p src/jni/app src/jni/SDL
cp "$APPLICATION_MK" src/jni/Application.mk
cp "$TOP_ANDROID_MK" src/jni/Android.mk
cp "$APP_DIR/jni/Android.mk" src/jni/app/Android.mk
cp -r "$APP_SRC_DIR"/src src/jni/app/src
sed -e "s/@VERSION_NAME@/$APP_VERSION/" -e "s/@VERSION_CODE@/$VERSION_CODE/" \
  -e "s/@MIN_SDK@/$PACKAGE_PLATFORM/" -e "s/@APP_ID@/$APP_ID/" \
  -e "s/@APP_LABEL@/$APP_LABEL/" \
  "$APP_DIR/AndroidManifest.xml" > src/AndroidManifest.xml

# Launcher icons: mdpi 48 px up to xxxhdpi 192 px (INT-4).
for d in mdpi:48 hdpi:72 xhdpi:96 xxhdpi:144 xxxhdpi:192; do
  mkdir -p "src/res/mipmap-${d%%:*}"
  cp "$APP_SRC_DIR/data/icons/${d##*:}/$PROGRAM.png" "src/res/mipmap-${d%%:*}/ic_launcher.png"
done

# SDL, prebuilt; its include directory as SDL ships it (SDL2: SDL.h at the
# top, SDL3: SDL3/SDL.h).
cat > src/jni/SDL/Android.mk <<MK
LOCAL_PATH := \$(call my-dir)
include \$(CLEAR_VARS)
LOCAL_MODULE := SDL$SDL_MAJOR
LOCAL_SRC_FILES := $SDL_ANDROID_LIBS/lib/\$(TARGET_ARCH_ABI)/libSDL$SDL_MAJOR.so
LOCAL_EXPORT_C_INCLUDES := $SDL_ANDROID_LIBS/include
include \$(PREBUILT_SHARED_LIBRARY)
MK
chmod -R u+rwX src

mkdir -p assets
cp "$GAMECONTROLLERDB" assets/gamecontrollerdb.txt

"$NDK/ndk-build" \
  NDK_PROJECT_PATH="$PWD/src" \
  APP_BUILD_SCRIPT="$PWD/src/jni/Android.mk" \
  NDK_APPLICATION_MK="$PWD/src/jni/Application.mk" \
  SDL_MAJOR="$SDL_MAJOR" \
  SDL_JSTEST_VERSION="$APP_VERSION" \
  -j"${NIX_BUILD_CORES:-$(nproc)}"

mkdir -p out
"$BT/aapt" package -f -M src/AndroidManifest.xml -S src/res -A assets \
  -I "$PACKAGE_JAR" -F out/base.apk
cp "$SDL_ANDROID_LIBS/dex/classes.dex" out/classes.dex
for abi in $TARGET_ABIS; do
  mkdir -p "out/lib/$abi"
  cp src/libs/"$abi"/*.so "out/lib/$abi/"
done
( cd out && "$BT/aapt" add base.apk classes.dex && zip -r base.apk lib )

"$BT/zipalign" -f 4 out/base.apk out/aligned.apk
cp "$KEYSTORE" debug.keystore
"$BT/apksigner" sign --ks debug.keystore --ks-pass pass:android --key-pass pass:android \
  --out "out/$PROGRAM.apk" out/aligned.apk
"$BT/aapt" dump badging "out/$PROGRAM.apk" | head -5
