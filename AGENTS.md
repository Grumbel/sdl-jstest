# AGENTS.md

Guidance for AI agents and automated assistants working on **sdl-jstest**.

## Project overview

`sdl-jstest`, `sdl2-jstest`, and `sdl3-jstest` are small C programs that list and test joysticks via SDL 1.2, SDL 2, and SDL 3. They show axes, buttons, hats, balls, GameController mappings, sensors, rumble, and (for DualSense) trigger effects. UI is ncurses-based. Build system is CMake; optional Nix flake support is included.

Key paths:

| Path | Role |
|------|------|
| `src/sdl-jstest.c` | SDL 1.2 joystick tester |
| `src/sdl2-jstest.c` | SDL 2 joystick / GameController tester |
| `src/sdl3-jstest.c` | SDL 3 joystick / Gamepad tester |
| `src/utils/dualsense.h` | DualSense (PS5) trigger-effect helpers |
| `CMakeLists.txt` | Build and install rules |
| `external/sdl_gamecontrollerdb/` | Submodule: GameControllerDB mappings |
| `sdl-jstest.nix` / `flake.nix` | Nix packaging |

License: GPL-3.0-or-later. Version is read from `VERSION`.

## Working conventions

- Prefer small, focused changes. Do not refactor unrelated code.
- Keep C99 style consistent with existing sources (includes, error handling, ncurses usage).
- Preserve GPL-3.0 headers and copyright notices on modified files.
- When touching build or packaging, keep both CMake and Nix paths working if possible.
- Submodule `external/sdl_gamecontrollerdb` must remain a proper git submodule (do not vendor its contents into this tree unless explicitly requested).
- Man pages (`.1`), SVG icons, and screenshots are part of the project; update them when CLI behavior or branding changes.

## Build reminders

```sh
git submodule init && git submodule update
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
