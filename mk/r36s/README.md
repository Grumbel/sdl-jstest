<!-- SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com> -->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# sdl-jstest for the R36S (ArkOS) and other PortMaster handhelds

Three programs that show what SDL sees of the built-in controls and of
any USB / Bluetooth gamepad: axes, buttons, hats, the gamepad mapping, the
events as they arrive.

| Launcher | Program | SDL |
|----------|---------|-----|
| `SDL1 Joystick Test.sh` | `sdl1-jstest` | SDL 1.2 API through sdl12-compat (shipped in `libs/`) on the device's SDL2 |
| `SDL2 Joystick Test.sh` | `sdl2-jstest` | the device's SDL2, the newest one ArkOS has installed (needs 2.0.18 or newer) |
| `SDL3 Joystick Test.sh` | `sdl3-jstest` | SDL3 with KMS/DRM, shipped in `libs/` |

Built natively for aarch64 against an ArkOS-era (Ubuntu 19.10) userland,
so they need nothing newer than the device's glibc 2.30.

## Install

Copy the three `.sh` files and the `sdl-jstest/` folder to `/roms/ports/`,
or put the zip into `ports/PortMaster/autoinstall/` and start PortMaster
once.

## Controls

There is no keyboard on the device and no gptokeyb: it would turn the
buttons under test into key presses.

| Input | Action |
|-------|--------|
| D-pad / stick | Move through the device menu |
| Any button | Next row in the device menu |
| Hold a button 1 s | Open the selected device (or Quit) |
| Hold any button 2 s | Leave a test view, back to the menu |
| Touch / mouse | Click a row |

## Logs

Each launcher writes `sdl-jstest/log-<program>.txt`: the startup report
(SDL version and library file, video and render drivers, every joystick
with its GUID and where its mapping came from) and the event log.

Over ssh the programs also run in the terminal:

```bash
cd /roms/ports/sdl-jstest
LD_LIBRARY_PATH=libs ./sdl3-jstest --list
LD_LIBRARY_PATH=libs ./sdl3-jstest --test 0
```

## Layout

```
/roms/ports/SDL1 Joystick Test.sh
/roms/ports/SDL2 Joystick Test.sh
/roms/ports/SDL3 Joystick Test.sh
/roms/ports/sdl-jstest/launch.sh                # shared by the three
/roms/ports/sdl-jstest/sdl{1,2,3}-jstest
/roms/ports/sdl-jstest/gamecontrollerdb.txt
/roms/ports/sdl-jstest/libs/libSDL-1.2.so.0     # sdl12-compat
/roms/ports/sdl-jstest/libs/libSDL3.so.0
```

## Build (from the source tree)

```bash
nix build .#sdl-jstest-r36s
nix build .#sdl-jstest-r36s-portmaster
nix build .#sdl-jstest-r36s-portmaster-zip
```
