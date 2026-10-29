#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Builds SDL's native library (SDL2 or SDL3) with ndk-build for every ABI,
# and its Java side (SDLActivity) into classes.dex, then collects them into
# $out: lib/<abi>/libSDLn.so, include/ (SDL's include directory as is) and
# dex/classes.dex. After xanatoo / Pingus.
#
# Env: ANDROID_HOME, BUILD_TOOLS_VERSION, COMPILE_PLATFORM, PACKAGE_PLATFORM,
#      SDL_SRC (SDL source tree), APPLICATION_MK, TOP_ANDROID_MK,
#      TARGET_ABIS, out
set -euo pipefail

# shellcheck source=ndk.sh
. "$(dirname "$0")/ndk.sh"
NDK="$(resolve_ndk)"
echo "==> NDK=$NDK"
BT="$ANDROID_HOME/build-tools/$BUILD_TOOLS_VERSION"
COMPILE_JAR="$ANDROID_HOME/platforms/android-$COMPILE_PLATFORM/android.jar"

mkdir -p sdl-jni
cp -r "$SDL_SRC" sdl-jni/SDL
chmod -R u+rwX sdl-jni/SDL
# NDK r26+ makes ALooper_pollAll a hard error; older SDL2 releases call it
# in the sensor code. ALooper_pollOnce is the supported replacement.
for f in $(grep -Rl 'ALooper_pollAll' sdl-jni/SDL/src || true); do
  sed -i 's/ALooper_pollAll/ALooper_pollOnce/g' "$f"
done
cp "$APPLICATION_MK" sdl-jni/Application.mk
cp "$TOP_ANDROID_MK" sdl-jni/Android.mk

"$NDK/ndk-build" \
  NDK_PROJECT_PATH="$PWD/sdl-jni" \
  APP_BUILD_SCRIPT="$PWD/sdl-jni/Android.mk" \
  NDK_APPLICATION_MK="$PWD/sdl-jni/Application.mk" \
  -j"${NIX_BUILD_CORES:-$(nproc)}"

mkdir -p javasrc classes
cp -r sdl-jni/SDL/android-project/app/src/main/java/org javasrc/org
javac -encoding UTF-8 --release 8 -nowarn -classpath "$COMPILE_JAR" -d classes $(find javasrc -name '*.java')
"$BT/d8" --output classes --min-api "$PACKAGE_PLATFORM" --lib "$COMPILE_JAR" $(find classes -name '*.class')

mkdir -p "$out/lib" "$out/dex" "$out/include"
for abi in $TARGET_ABIS; do
  mkdir -p "$out/lib/$abi"
  cp sdl-jni/libs/"$abi"/libSDL*.so "$out/lib/$abi/"
done
cp -r sdl-jni/SDL/include/. "$out/include/"
cp classes/classes.dex "$out/dex/classes.dex"
