# TODO / handoff

## Tip
Work line base: `cf4fab3`
Current tip: see `git log -1`

## Done this session
Optional SDL window joystick test (`-G` / `--gui [JOYNUM]`) for all three binaries.

### Shared
- `src/ui/graph_ui.{h,c}` — ARGB8888 software UI (axes, buttons, hats, balls, event log, menu)
- CMakeLists links `graph_ui.c` into sdl1/2/3-jstest

### Per-binary presenters + CLI
- **sdl2**: `GraphPresent2` (Window/Renderer/Texture), `gui_test_joystick`, `gui_test_gamecontroller`, `gui_interactive_menu`
- **sdl3**: `GraphPresent3`, `gui_test_joystick`, `gui_interactive_menu`
- **sdl1**: `GraphPresent1` (SDL_SetVideoMode + surface blit), `gui_test_joystick`, `gui_interactive_menu`

Usage:
```
sdl2-jstest --gui          # device menu in SDL window
sdl2-jstest --gui 0        # test device 0 in window
sdl2-jstest --test 0       # existing ncurses path unchanged
```

## Follow-ups (optional)
- Man pages for `--gui`
- SDL3 gamepad-labelled button groups in GUI (currently raw joystick view for `--gui N`)
- Restore DualSense trigger-effect cycling in SDL2 gamecontroller GUI path if desired
- Resize window → recreate texture / letterbox (currently fixed 960×720 buffer stretched by renderer)
