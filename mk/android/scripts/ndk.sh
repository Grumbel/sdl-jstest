# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# resolve_ndk: the NDK under ANDROID_NDK_HOME or ANDROID_HOME (ndk-bundle,
# or the newest ndk/<version>, or ANDROID_NDK_VERSION if set).
resolve_ndk() {
  if [ -n "${ANDROID_NDK_HOME:-}" ] && [ -x "$ANDROID_NDK_HOME/ndk-build" ]; then
    printf '%s' "$ANDROID_NDK_HOME"
    return
  fi
  if [ -z "${ANDROID_HOME:-}" ]; then
    echo "error: ANDROID_HOME is not set" >&2
    exit 1
  fi
  if [ -x "$ANDROID_HOME/ndk-bundle/ndk-build" ]; then
    printf '%s' "$ANDROID_HOME/ndk-bundle"
    return
  fi
  if [ -n "${ANDROID_NDK_VERSION:-}" ] && [ -x "$ANDROID_HOME/ndk/$ANDROID_NDK_VERSION/ndk-build" ]; then
    printf '%s' "$ANDROID_HOME/ndk/$ANDROID_NDK_VERSION"
    return
  fi
  newest=
  for d in "$ANDROID_HOME"/ndk/*; do
    [ -x "$d/ndk-build" ] && newest=$d
  done
  if [ -n "$newest" ]; then
    printf '%s' "$newest"
    return
  fi
  echo "error: no ndk-build under ANDROID_HOME=$ANDROID_HOME" >&2
  exit 1
}
