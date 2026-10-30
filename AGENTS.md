# AGENTS.md

Guidance for AI agents and automated assistants working on **sdl-jstest**.

## Project overview

`sdl1-jstest`, `sdl2-jstest`, and `sdl3-jstest` are small C programs that list and test joysticks via SDL 1.2, SDL 2, and SDL 3. They show axes, buttons, hats, balls, GameController mappings, sensors, rumble, and (for DualSense) trigger effects. The terminal UI is ncurses-based (optional); the window UI (`--gui`) is the default without it. Build system is CMake; the Nix flake builds every port and follows gamepkgstd (`~/projects/gamepkgstd/trunk/gamepkgstd`).

Key paths:

| Path | Role |
|------|------|
| `src/sdl1-jstest.c` | SDL 1.2 joystick tester |
| `src/sdl2-jstest.c` | SDL 2 joystick / GameController tester |
| `src/sdl3-jstest.c` | SDL 3 joystick / Gamepad tester |
| `src/ui/test_ui.*` | Shared ncurses test UI (split panes, event log) |
| `src/ui/graph_ui.*` | Shared software-framebuffer SDL window test UI |
| `src/ui/gui_nav.*` | Window UI navigation helpers (hold-to-leave, stick as D-pad) |
| `src/common/cli.*` | Option parsing (before SDL_Init) |
| `src/common/report.*` | Startup report on stderr, data directory, platform glue (Windows console, Android logcat) |
| `src/utils/dualsense.h` | DualSense (PS5) trigger-effect helpers |
| `doc/` | Man pages and screenshots |
| `data/` | Application icons (SVG) |
| `CMakeLists.txt` | Build and install rules |
| `external/sdl_gamecontrollerdb/` | git subtree of SDL_GameControllerDB (`gamecontrollerdb.txt`) |
| `sdl-jstest.nix` / `flake.nix` | Nix packaging, checks, dev shell |
| `nix/r36s.nix`, `mk/r36s/` | R36S / ArkOS PortMaster port (eoan sysroot, cross GCC) |
| `nix/android.nix`, `mk/android/` | Android APKs of sdl2-jstest / sdl3-jstest (ndk-build) |
| `nix/windows.nix`, `data/windows/` | Windows builds (mingw-w64), SDL built from source |
| `nix/wasm.nix`, `mk/wasm/` | WebAssembly build of sdl2-jstest / sdl3-jstest |
| `tools/make_icons.py` | Generates the PNG / ICO icons under `data/icons/` |

License: GPL-3.0-or-later. Version is read from `VERSION`.

## Working conventions

- Prefer small, focused changes. Do not refactor unrelated code.
- Keep C99 style consistent with existing sources (includes, error handling, ncurses usage).
- Preserve GPL-3.0 headers and copyright notices on modified files.
- When touching build or packaging, keep both CMake and Nix paths working if possible.
- `external/sdl_gamecontrollerdb` is a squashed git subtree of
  https://github.com/gabomdq/SDL_GameControllerDB; update it with
  `git subtree pull --prefix external/sdl_gamecontrollerdb https://github.com/gabomdq/SDL_GameControllerDB master --squash`,
  never edit files in it. Nix builds use the `sdl_gamecontrollerdb` flake
  input instead (`nix flake update sdl_gamecontrollerdb`).
- Man pages and screenshots live under `doc/`; SVG icons under `data/`. Update them when CLI behavior or branding changes.
- The window UI runs one frame per `gui_frame()` call (menu or device
  view in a `GuiSession`); never add a blocking loop there, the browser
  build depends on it.
- Never hard-code the version: `VERSION` is the only place (gamepkgstd VER-5).
- New options go into `cli.c`, its usage text and the three man pages.
- R36S launcher (`mk/r36s/launch.sh`): never LD_PRELOAD SDL2. Its symbols
  share names with SDL 1.2 (`SDL_PollEvent`, `SDL_JoystickOpen`, ...) and
  would replace sdl12-compat's in sdl1-jstest; pick an SDL2 through
  LD_LIBRARY_PATH (sdl12-compat dlopen()s `libSDL2-2.0.so.0`).

## Build reminders

```sh
mkdir -p build && cd build
cmake ..
make
```

Dependencies: cmake, libsdl1.2-dev, libsdl2-dev, libncurses (ncursesw preferred).

## Chat / agent workflow rule

**Every change made in a chat session must end with a detailed git commit message summarizing what was done.**

After completing work (code edits, docs, build fixes, etc.), output a ready-to-use commit message that:

1. Starts with a concise subject line (≤ ~72 characters, imperative mood).
2. Includes a body that explains *what* changed and *why*, listing notable files or behaviors when helpful.
3. Is suitable to paste as `git commit -m "..."` or into an editor for a multi-line message.

Do not skip this step even for documentation-only or one-line fixes.
