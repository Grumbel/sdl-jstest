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

## Earlier: optional SDL window joystick test (`-G` / `--gui [JOYNUM]`)

- `src/ui/graph_ui.{h,c}` — ARGB8888 software UI (axes, buttons, hats,
  balls, event log, menu), linked into sdl1/2/3-jstest
- Follow-ups: SDL3 gamepad-labelled button groups in GUI, DualSense
  trigger-effect cycling in the SDL2 GUI path
