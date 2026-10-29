#!/bin/bash
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# sdl-jstest: PortMaster launcher for the R36S / ArkOS (and similar
# handhelds). Usage: launch.sh sdl1-jstest|sdl2-jstest|sdl3-jstest [ARGS]

PROGRAM="${1:-sdl2-jstest}"
shift

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}
if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi
source "$controlfolder/control.txt"
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"
get_controls

GAMEDIR="/$directory/ports/sdl-jstest"
cd "$GAMEDIR" || exit 1
> "$GAMEDIR/log-$PROGRAM.txt" && exec > >(tee "$GAMEDIR/log-$PROGRAM.txt") 2>&1

# sdl12-compat (libSDL-1.2.so.0) and SDL3 (libSDL3.so.0) ship with the
# port; SDL2 is the device's.
export LD_LIBRARY_PATH="$GAMEDIR/libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# PortMaster's mapping for the built-in pad, on top of gamecontrollerdb.txt.
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"

# No gptokeyb: it would turn the buttons under test into key presses (B
# into Escape). The program quits by itself: Quit in the device menu, hold
# any button for two seconds to leave a test view.
pm_platform_helper "$GAMEDIR/$PROGRAM"

# ArkOS installs several SDL2 builds side by side but points
# libSDL2-2.0.so.0 at eoan's 2.0.10. Preload the newest one for sdl2-jstest
# and sdl1-jstest (sdl12-compat runs on it); the startup report in
# log-*.txt names the version in use.
sdl_preload=""
if [ "$PROGRAM" != "sdl3-jstest" ]; then
  sdl_default=$(readlink -f /usr/lib/aarch64-linux-gnu/libSDL2-2.0.so.0 2>/dev/null)
  sdl_newest=$(ls /usr/lib/aarch64-linux-gnu/libSDL2-2.0.so.0.* 2>/dev/null | sort -V | tail -n 1)
  if [ -n "$sdl_newest" ] && [ -n "$sdl_default" ] &&
     [ "$(printf '%s\n%s\n' "$sdl_default" "$sdl_newest" | sort -V | tail -n 1)" != "$sdl_default" ]; then
    echo "$PROGRAM: using $sdl_newest instead of $sdl_default"
    sdl_preload="$sdl_newest"
  fi
fi

LD_PRELOAD="$sdl_preload${LD_PRELOAD:+:$LD_PRELOAD}" \
  "./$PROGRAM" --gui --fullscreen --data-dir "$GAMEDIR" "$@"
pm_finish
