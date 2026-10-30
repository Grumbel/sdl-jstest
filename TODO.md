# TODO / handoff

## Current work line: ports + gamepkgstd (started 2026-10-29, base `fb44667`)

Goal: sdl-jstest on R36S (PortMaster), Android (SDL2 and SDL3 APKs),
Windows (incl. SDL3) next to the native build, following gamepkgstd
(`~/projects/gamepkgstd/trunk/gamepkgstd`). SDL3 on Android / R36S doubles
as a test of SDL3 on those targets for other projects (xanatoo).

Plan, one commit (or a few) per step:

1. Shared `src/common/`: GNU-style option parsing before SDL_Init (OPT-1,
   OPT-2), data directory for gamecontrollerdb.txt (DIR-3: `--data-dir`,
   `<exe>/../share/sdl-jstest`, Windows `data/`, compiled-in path; Android
   assets), startup report on stderr (LOG-1..7).
2. GUI for devices without keyboard: one window for the whole session,
   640×480 buffer scaled with aspect kept, `--fullscreen`, gamepad / touch /
   Android Back navigation, hotplug-aware device menu, ncurses optional
   (Android has none; GUI is the default then).
3. Packaging: VER-5 version, flake checks (FLAKE-9..12), dev shell (DEV),
   metainfo + PNG icons (INT).
4. R36S: eoan sysroot (xanatoo's `nix/r36s.nix`), SDL1 + SDL2 against the
   device libraries, SDL3 built for KMSDRM and shipped in `libs/`,
   PortMaster port with one launcher per binary.
5. Android: `sdl2-jstest-android`, `sdl3-jstest-android` (ndk-build after
   xanatoo's `nix/android.nix`).
6. Windows SDL3.
7. Web (wasm) — only if the blocking loops can be handled (WEB-1/3).

Status (2026-10-30): all seven steps done; `nix flake check` passes.

- R36S: tested on r36s-black (ArkOS, SDL2 2.30.10 picked by the
  launcher): all three start fullscreen on KMSDRM, sdl2 / sdl3 on GLES2
  (Mali-G31), the built-in pad with PortMaster's mapping. Not yet done:
  pressing buttons in the device views on the device, installing through
  PortMaster itself.
- R36S, sdl1-jstest (fixed 2026-10-30): no button worked in the menu and
  the pad's name was "(null)". The launcher LD_PRELOADed the newest SDL2,
  whose SDL_PollEvent / SDL_JoystickName / SDL_JoystickOpen ... (same
  names as SDL 1.2) then replaced sdl12-compat's, so the program got SDL2
  events. The launcher now puts a libSDL2-2.0.so.0 symlink to it first on
  LD_LIBRARY_PATH instead; sdl12-compat dlopen()s that name.
- sdl2-jstest on R36S is built against newer SDL2 headers (sdl2-compat's)
  with a link stub and refuses SDL < 2.0.18 at run time; the launcher
  picks the newest SDL2 ArkOS ships. This departs from ARK-3 (build
  against the device's SDL2): the code needs 2.0.18 API.
- Android: both APKs tested on a Fire 7 (Fire OS 5.1.1): menu, the
  accelerometer as a joystick (SDL2), Back, Quit, report in logcat.
- Windows: win32 / win64 under Wine on Xvfb: --version, --list, window UI.
- Web: sdl2-jstest / sdl3-jstest only (sdl12-compat needs dlopen). The
  window UI runs one frame per call (gui_frame), so no Asyncify. Tested in
  Chromium without a gamepad; a real pad in a browser is still to do.
  Nothing is stored (no IDBFS) and there is nothing to pause (WEB-10).
- Follow-ups: Android adaptive icon (INT-14, "should").

## Earlier: optional SDL window joystick test (`-G` / `--gui [JOYNUM]`)

- `src/ui/graph_ui.{h,c}` — ARGB8888 software UI (axes, buttons, hats,
  balls, event log, menu), linked into sdl1/2/3-jstest
- Follow-ups: SDL3 gamepad-labelled button groups in GUI, DualSense
  trigger-effect cycling in the SDL2 GUI path
