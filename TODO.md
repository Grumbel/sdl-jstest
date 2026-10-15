# TODO / handoff

## Tip
Base: `cf4fab3` (origin/master at start of this work line).

## Goal
Optional SDL graphical joystick test for sdl1/2/3-jstest — same info as ncurses UI
(axes bars, buttons, hats, balls, event log, device menu), drawn in an SDL window.

## Done
- Shared software-framebuffer UI: `src/ui/graph_ui.h`, `src/ui/graph_ui.c`
  - ARGB8888 buffer, embedded 8x8 font (no SDL_ttf)
  - `graph_ui_draw_joystick`, `graph_ui_draw_menu`, `graph_ui_log`, menu keys
- CMakeLists.txt links `graph_ui.c` into all three targets

## Remaining
Wire `-G/--gui [JOYNUM]` into each of:
- `src/sdl1-jstest.c` — SDL_SetVideoMode / surface blit of `ui.pixels`
- `src/sdl2-jstest.c` — SDL_Window + Renderer + streaming texture
- `src/sdl3-jstest.c` — SDL3 CreateWindow/Renderer/Texture + RenderTexture

Pattern (already designed):
1. `graph_ui_init` → present each frame by uploading `ui.pixels`
2. On joystick events, `graph_ui_log` + redraw
3. Esc/q → back to menu; window close → quit
4. `--gui` alone → interactive menu in window; `--gui N` → test device N
5. Keep existing `-t/--test` as terminal (ncurses) path

Help text should distinguish terminal vs SDL window modes.
