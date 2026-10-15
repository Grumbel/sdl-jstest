// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <SDL3/SDL.h>
#include <assert.h>
#include <curses.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define DUALSENSE_GAMEPAD_TYPE SDL_Gamepad
#define DUALSENSE_SEND_EFFECT  SDL_SendGamepadEffect
#include "utils/dualsense.h"
#include "ui/test_ui.h"
#include "ui/graph_ui.h"
#include <stdint.h>



typedef struct GraphPresent3 {
    SDL_Window* win; SDL_Renderer* ren; SDL_Texture* tex; GraphUI ui;
} GraphPresent3;
static int graph_present3_open(GraphPresent3* gp, const char* title)
{
    memset(gp, 0, sizeof(*gp));
    if (!graph_ui_init(&gp->ui, GRAPH_UI_DEFAULT_W, GRAPH_UI_DEFAULT_H)) return 0;
    gp->win = SDL_CreateWindow(title ? title : "sdl3-jstest", gp->ui.w, gp->ui.h, SDL_WINDOW_RESIZABLE);
    if (!gp->win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); graph_ui_shutdown(&gp->ui); return 0; }
    gp->ren = SDL_CreateRenderer(gp->win, NULL);
    if (!gp->ren) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); SDL_DestroyWindow(gp->win); graph_ui_shutdown(&gp->ui); return 0; }
    gp->tex = SDL_CreateTexture(gp->ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, gp->ui.w, gp->ui.h);
    if (!gp->tex) { fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError()); SDL_DestroyRenderer(gp->ren); SDL_DestroyWindow(gp->win); graph_ui_shutdown(&gp->ui); return 0; }
    return 1;
}
static void graph_present3_frame(GraphPresent3* gp)
{
    SDL_UpdateTexture(gp->tex, NULL, gp->ui.pixels, gp->ui.w * (int)sizeof(uint32_t));
    SDL_RenderClear(gp->ren); SDL_RenderTexture(gp->ren, gp->tex, NULL, NULL); SDL_RenderPresent(gp->ren);
}
static void graph_present3_close(GraphPresent3* gp)
{
    if (gp->tex) SDL_DestroyTexture(gp->tex);
    if (gp->ren) SDL_DestroyRenderer(gp->ren);
    if (gp->win) SDL_DestroyWindow(gp->win);
    graph_ui_shutdown(&gp->ui); memset(gp, 0, sizeof(*gp));
}
static int graph_present3_map_key(const SDL_Event* e, int* mk, TestUIAction* act)
{
    if (e->type != SDL_EVENT_KEY_DOWN) return 0;
    SDL_Keycode k = e->key.key;
    if (k == SDLK_ESCAPE || k == SDLK_Q) { if (act) *act = TEST_UI_BACK; if (mk) *mk = 27; return 1; }
    if (k == SDLK_C && (e->key.mod & SDL_KMOD_CTRL)) { if (act) *act = TEST_UI_QUIT; if (mk) *mk = 27; return 1; }
    if (!mk) return 0;
    if (k == SDLK_UP || k == SDLK_K) *mk = GRAPH_UI_KEY_UP;
    else if (k == SDLK_DOWN || k == SDLK_J) *mk = GRAPH_UI_KEY_DOWN;
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) *mk = '\n';
    else if (k >= SDLK_0 && k <= SDLK_9) *mk = '0' + (int)(k - SDLK_0);
    else return 0;
    return 1;
}

static const char* sdl_event_name(Uint32 type)
{
    switch (type)
    {
#define E(x) case x: return #x
    E(SDL_EVENT_QUIT);
    E(SDL_EVENT_TERMINATING);
    E(SDL_EVENT_LOW_MEMORY);
    E(SDL_EVENT_WILL_ENTER_BACKGROUND);
    E(SDL_EVENT_DID_ENTER_BACKGROUND);
    E(SDL_EVENT_WILL_ENTER_FOREGROUND);
    E(SDL_EVENT_DID_ENTER_FOREGROUND);
    E(SDL_EVENT_LOCALE_CHANGED);
    E(SDL_EVENT_SYSTEM_THEME_CHANGED);
    E(SDL_EVENT_DISPLAY_ORIENTATION);
    E(SDL_EVENT_DISPLAY_ADDED);
    E(SDL_EVENT_DISPLAY_REMOVED);
    E(SDL_EVENT_DISPLAY_MOVED);
    E(SDL_EVENT_WINDOW_SHOWN);
    E(SDL_EVENT_WINDOW_HIDDEN);
    E(SDL_EVENT_WINDOW_EXPOSED);
    E(SDL_EVENT_WINDOW_MOVED);
    E(SDL_EVENT_WINDOW_RESIZED);
    E(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED);
    E(SDL_EVENT_WINDOW_MINIMIZED);
    E(SDL_EVENT_WINDOW_MAXIMIZED);
    E(SDL_EVENT_WINDOW_RESTORED);
    E(SDL_EVENT_WINDOW_MOUSE_ENTER);
    E(SDL_EVENT_WINDOW_MOUSE_LEAVE);
    E(SDL_EVENT_WINDOW_FOCUS_GAINED);
    E(SDL_EVENT_WINDOW_FOCUS_LOST);
    E(SDL_EVENT_WINDOW_CLOSE_REQUESTED);
    E(SDL_EVENT_KEY_DOWN);
    E(SDL_EVENT_KEY_UP);
    E(SDL_EVENT_TEXT_EDITING);
    E(SDL_EVENT_TEXT_INPUT);
    E(SDL_EVENT_KEYMAP_CHANGED);
    E(SDL_EVENT_KEYBOARD_ADDED);
    E(SDL_EVENT_KEYBOARD_REMOVED);
    E(SDL_EVENT_MOUSE_MOTION);
    E(SDL_EVENT_MOUSE_BUTTON_DOWN);
    E(SDL_EVENT_MOUSE_BUTTON_UP);
    E(SDL_EVENT_MOUSE_WHEEL);
    E(SDL_EVENT_MOUSE_ADDED);
    E(SDL_EVENT_MOUSE_REMOVED);
    E(SDL_EVENT_JOYSTICK_AXIS_MOTION);
    E(SDL_EVENT_JOYSTICK_BALL_MOTION);
    E(SDL_EVENT_JOYSTICK_HAT_MOTION);
    E(SDL_EVENT_JOYSTICK_BUTTON_DOWN);
    E(SDL_EVENT_JOYSTICK_BUTTON_UP);
    E(SDL_EVENT_JOYSTICK_ADDED);
    E(SDL_EVENT_JOYSTICK_REMOVED);
    E(SDL_EVENT_JOYSTICK_BATTERY_UPDATED);
    E(SDL_EVENT_JOYSTICK_UPDATE_COMPLETE);
    E(SDL_EVENT_GAMEPAD_AXIS_MOTION);
    E(SDL_EVENT_GAMEPAD_BUTTON_DOWN);
    E(SDL_EVENT_GAMEPAD_BUTTON_UP);
    E(SDL_EVENT_GAMEPAD_ADDED);
    E(SDL_EVENT_GAMEPAD_REMOVED);
    E(SDL_EVENT_GAMEPAD_REMAPPED);
    E(SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN);
    E(SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION);
    E(SDL_EVENT_GAMEPAD_TOUCHPAD_UP);
    E(SDL_EVENT_GAMEPAD_SENSOR_UPDATE);
    E(SDL_EVENT_GAMEPAD_UPDATE_COMPLETE);
    E(SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED);
    E(SDL_EVENT_FINGER_DOWN);
    E(SDL_EVENT_FINGER_UP);
    E(SDL_EVENT_FINGER_MOTION);
    E(SDL_EVENT_CLIPBOARD_UPDATE);
    E(SDL_EVENT_DROP_FILE);
    E(SDL_EVENT_DROP_TEXT);
    E(SDL_EVENT_DROP_BEGIN);
    E(SDL_EVENT_DROP_COMPLETE);
    E(SDL_EVENT_AUDIO_DEVICE_ADDED);
    E(SDL_EVENT_AUDIO_DEVICE_REMOVED);
    E(SDL_EVENT_SENSOR_UPDATE);
    E(SDL_EVENT_PEN_DOWN);
    E(SDL_EVENT_PEN_UP);
    E(SDL_EVENT_PEN_MOTION);
    E(SDL_EVENT_PEN_BUTTON_DOWN);
    E(SDL_EVENT_PEN_BUTTON_UP);
    E(SDL_EVENT_CAMERA_DEVICE_ADDED);
    E(SDL_EVENT_CAMERA_DEVICE_REMOVED);
    E(SDL_EVENT_RENDER_TARGETS_RESET);
    E(SDL_EVENT_RENDER_DEVICE_RESET);
    E(SDL_EVENT_POLL_SENTINEL);
#undef E
    default:
    {
        static char buf[48];
        snprintf(buf, sizeof(buf), "UNKNOWN(%u)", (unsigned)type);
        return buf;
    }
    }
}



/* Map a user-facing index (as in --list order) to an SDL_JoystickID. */
static SDL_JoystickID joystick_id_at_index(int idx)
{
    int count = 0;
    SDL_JoystickID* ids = SDL_GetJoysticks(&count);
    if (!ids || idx < 0 || idx >= count)
    {
        SDL_free(ids);
        return 0;
    }
    SDL_JoystickID id = ids[idx];
    SDL_free(ids);
    return id;
}

static int num_joysticks(void)
{
    int count = 0;
    SDL_JoystickID* ids = SDL_GetJoysticks(&count);
    SDL_free(ids);
    return count;
}


static int str2int(const char* str, int* val)
{
    char* endptr;

    errno = 0;
    long tmp = strtol(str, &endptr, 10);

    // error
    if (errno != 0)
    {
        return 0;
    }

    // garbage at the end
    if (*endptr != '\0')
    {
        return 0;
    }

    // out of range of int
    if (tmp < INT_MIN || tmp > INT_MAX)
    {
        return 0;
    }

    *val = (int)tmp;
    return 1;
}

static void print_joystick_info(int joy_idx, SDL_Joystick* joy, SDL_Gamepad* gamepad)
{
    SDL_GUID guid = SDL_GetJoystickGUID(joy);
    char guid_str[64];
    SDL_GUIDToString(guid, guid_str, sizeof(guid_str));

    printf("Joystick Name:     '%s'\n", SDL_GetJoystickName(joy));
    printf("Joystick GUID:     %s\n", guid_str);
    printf("Joystick Number:   %2d\n", joy_idx);
    printf("Number of Axes:    %2d\n", SDL_GetNumJoystickAxes(joy));
    printf("Number of Buttons: %2d\n", SDL_GetNumJoystickButtons(joy));
    printf("Number of Hats:    %2d\n", SDL_GetNumJoystickHats(joy));
    printf("Number of Balls:   %2d\n", SDL_GetNumJoystickBalls(joy));
    printf("GameControllerConfig:\n");
    if (!gamepad)
    {
        printf("  missing (see 'gamecontrollerdb.txt' or SDL_GAMECONTROLLERCONFIG)\n");
    }
    else
    {
        printf("  Name:    '%s'\n", SDL_GetGamepadName(gamepad));
        printf("  Mapping: '%s'\n", SDL_GetGamepadMappingForGUID(guid));
    }
    printf("\n");
}

static void print_help(const char* prg)
{
    printf("Usage: %s [OPTION]\n", prg);
  printf("With no options, opens an interactive device menu.\n");
    printf("List available joysticks or test a joystick.\n");
    printf("This programm uses SDL for doing its test instead of using the raw\n"
        "/dev/input/jsX interface\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help             Print this help\n");
    printf("  --version              Print version number and exit\n");
    printf("  -l, --list             Search for available joysticks and list their properties\n");
    printf("  -t, --test JOYNUM      Terminal (ncurses) joystick state display\n");
    printf("  -G, --gui [JOYNUM]     SDL window joystick test (omit JOYNUM for menu)\n");
    printf("  -g, --gamecontroller IDX\n"
        "                         Test GameController\n");
    printf("  -e, --event JOYNUM     Display the events that are received from the joystick\n");
    printf("  -r, --rumble JOYNUM    Test rumble effects on gamepad JOYNUM\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s --list\n", prg);
    printf("  %s --test 1\n", prg);
    printf("  %s --gui\n", prg);
    printf("  %s --gui 0\n", prg);
}

static void list_joysticks(void)
{
    int n = num_joysticks();
    if (n == 0)
    {
        printf("No joysticks were found\n");
    }
    else
    {
        printf("Found %d joystick(s)\n\n", n);
        for (int joy_idx = 0; joy_idx < n; ++joy_idx)
        {
            SDL_JoystickID id = joystick_id_at_index(joy_idx);
            SDL_Joystick* joy = SDL_OpenJoystick(id);
            if (!joy)
            {
                fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
            }
            else
            {
                SDL_Gamepad* gamepad = SDL_IsGamepad(id) ? SDL_OpenGamepad(id) : NULL;
                print_joystick_info(joy_idx, joy, gamepad);
                if (gamepad)
                {
                    SDL_CloseGamepad(gamepad);
                }
                SDL_CloseJoystick(joy);
            }
        }
    }
}

static void test_joystick(int joy_idx)
{
    SDL_JoystickID id = joystick_id_at_index(joy_idx);
    SDL_Joystick* joy = SDL_OpenJoystick(id);
    if (!joy)
    {
        fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
        return;
    }

    int num_axes = SDL_GetNumJoystickAxes(joy);
    int num_buttons = SDL_GetNumJoystickButtons(joy);
    int num_hats = SDL_GetNumJoystickHats(joy);
    int num_balls = SDL_GetNumJoystickBalls(joy);
    if (num_axes < 0 || num_buttons < 0 || num_hats < 0 || num_balls < 0)
    {
        fprintf(stderr, "Unable to query joystick layout: %s\n", SDL_GetError());
        SDL_CloseJoystick(joy);
        exit(EXIT_FAILURE);
    }

    Sint16* axes = calloc((size_t)num_axes, sizeof(Sint16));
    Uint8* buttons = calloc((size_t)num_buttons, sizeof(Uint8));
    Uint8* hats = calloc((size_t)num_hats, sizeof(Uint8));
    Sint16* balls = calloc((size_t)num_balls, 2 * sizeof(Sint16));

    for (int i = 0; i < num_axes; ++i)
        axes[i] = SDL_GetJoystickAxis(joy, i);
    for (int i = 0; i < num_buttons; ++i)
        buttons[i] = SDL_GetJoystickButton(joy, i) ? 1 : 0;
    for (int i = 0; i < num_hats; ++i)
        hats[i] = SDL_GetJoystickHat(joy, i);

    SDL_JoystickID open_iid = id;

    TestUI ui;
    test_ui_init(&ui);
    test_ui_log(&ui, "Opened joystick %d id=%u '%s'", joy_idx, (unsigned)id,
                SDL_GetJoystickName(joy) ? SDL_GetJoystickName(joy) : "?");

    int quit = 0;
    SDL_Event event;
    while (!quit)
    {
        SDL_Delay(10);
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_JOYSTICK_AXIS_MOTION:
                if (event.jaxis.axis < num_axes)
                    axes[event.jaxis.axis] = event.jaxis.value;
                test_ui_log(&ui, "%s which=%d axis=%d value=%+d",
                            sdl_event_name(event.type),
                            event.jaxis.which, event.jaxis.axis, event.jaxis.value);
                break;

            case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
            case SDL_EVENT_JOYSTICK_BUTTON_UP:
                if (event.jbutton.button < num_buttons)
                    buttons[event.jbutton.button] = event.jbutton.down ? 1 : 0;
                test_ui_log(&ui, "%s which=%d button=%d %s",
                            sdl_event_name(event.type),
                            event.jbutton.which, event.jbutton.button,
                            event.jbutton.down ? "DOWN" : "UP");
                break;

            case SDL_EVENT_JOYSTICK_HAT_MOTION:
                if (event.jhat.hat < num_hats)
                    hats[event.jhat.hat] = event.jhat.value;
                test_ui_log(&ui, "HAT    joy=%d hat=%d value=%d",
                            event.jhat.which, event.jhat.hat, event.jhat.value);
                break;

            case SDL_EVENT_JOYSTICK_BALL_MOTION:
                if (event.jball.ball < num_balls)
                {
                    balls[2 * event.jball.ball + 0] = event.jball.xrel;
                    balls[2 * event.jball.ball + 1] = event.jball.yrel;
                }
                test_ui_log(&ui, "BALL   joy=%d ball=%d dx=%d dy=%d",
                            event.jball.which, event.jball.ball,
                            event.jball.xrel, event.jball.yrel);
                break;

            case SDL_EVENT_JOYSTICK_ADDED:
                test_ui_log(&ui, "DEVICE ADDED   which=%d", event.jdevice.which);
                break;

            case SDL_EVENT_JOYSTICK_REMOVED:
                test_ui_log(&ui, "*** DISCONNECTED *** %s which=%d%s",
                            sdl_event_name(event.type), event.jdevice.which,
                            event.jdevice.which == open_iid ? " (this device)" : "");
                if (event.jdevice.which == open_iid)
                {
                    test_ui_log(&ui, "*** Device unplugged — back to menu ***");
                    quit = 1;
                }
                break;

            case SDL_EVENT_GAMEPAD_ADDED:
                test_ui_log(&ui, "GAMEPAD ADDED  which=%d", event.gdevice.which);
                break;

            case SDL_EVENT_GAMEPAD_REMOVED:
                test_ui_log(&ui, "GAMEPAD REMOVED which=%d", event.gdevice.which);
                break;

            case SDL_EVENT_GAMEPAD_REMAPPED:
                test_ui_log(&ui, "GAMEPAD REMAPPED which=%d", event.gdevice.which);
                break;

            case SDL_EVENT_CLIPBOARD_UPDATE:
            case SDL_EVENT_KEYMAP_CHANGED:
                test_ui_log(&ui, "%s", sdl_event_name(event.type));
                break;

            case SDL_EVENT_QUIT:
                test_ui_log(&ui, "QUIT");
                quit = 1;
                break;

            default:
                test_ui_log(&ui, "%s", sdl_event_name(event.type));
                break;
            }
        }

        if (1) /* always paint; avoids blank screen until first event */
        {
            test_ui_draw_joystick(&ui, SDL_GetJoystickName(joy), joy_idx,
                                  axes, num_axes, buttons, num_buttons, NULL,
                                  NULL, 0,
                                  hats, num_hats, balls, num_balls);
            test_ui_refresh(&ui);
        }

        {
            TestUIAction act = test_ui_poll_action(&ui);
            if (act == TEST_UI_QUIT)
            {
                test_ui_shutdown(&ui);
                exit(EXIT_SUCCESS);
            }
            if (act == TEST_UI_BACK)
                quit = 1;
        }
    }

    test_ui_shutdown(&ui);
    free(balls);
    free(hats);
    free(buttons);
    free(axes);
    SDL_CloseJoystick(joy);
}

static Uint16 ConvertAxisToRumble(Sint16 axisval)
{
    /* Only start rumbling if the axis is past the halfway point */
    const Sint16 half_axis = (Sint16)SDL_ceilf(SDL_JOYSTICK_AXIS_MAX / 2.0f);
    if (axisval > half_axis)
    {
        return (Uint16)(axisval - half_axis) * 4;
    }
    else
    {
        return 0;
    }
}

static void test_gamecontroller_state(SDL_Gamepad* gamepad)
{
    assert(gamepad);

    int num_axes = SDL_GAMEPAD_AXIS_COUNT;
    int num_buttons = SDL_GAMEPAD_BUTTON_COUNT;
    Sint16* axes = calloc((size_t)num_axes, sizeof(Sint16));
    Uint8* buttons = calloc((size_t)num_buttons, sizeof(Uint8));
    const char** labels = calloc((size_t)num_buttons, sizeof(char*));
    for (int i = 0; i < num_buttons; ++i)
    {
        const char* s = SDL_GetGamepadStringForButton((SDL_GamepadButton)i);
        labels[i] = s ? s : "";
    }

    bool has_gyro = SDL_GamepadHasSensor(gamepad, SDL_SENSOR_GYRO);
    if (has_gyro)
        SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_GYRO, true);
    bool has_accel = SDL_GamepadHasSensor(gamepad, SDL_SENSOR_ACCEL);
    if (has_accel)
        SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_ACCEL, true);

    for (int i = 0; i < num_axes; ++i)
        axes[i] = SDL_GetGamepadAxis(gamepad, (SDL_GamepadAxis)i);
    for (int i = 0; i < num_buttons; ++i)
        buttons[i] = SDL_GetGamepadButton(gamepad, (SDL_GamepadButton)i) ? 1 : 0;

    SDL_Joystick* joy_ref = SDL_GetGamepadJoystick(gamepad);
    SDL_JoystickID open_iid = joy_ref ? SDL_GetJoystickID(joy_ref) : 0;

    TestUI ui;
    test_ui_init(&ui);
    test_ui_log(&ui, "Gamepad '%s' (id %u)",
                SDL_GetGamepadName(gamepad), (unsigned)open_iid);

    int quit = 0;
    SDL_Event event;
    while (!quit)
    {
        SDL_Delay(10);
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
                test_ui_log(&ui, "%s sensor=%d data=[%.3f %.3f %.3f]",
                            sdl_event_name(event.type), event.gsensor.sensor,
                            event.gsensor.data[0], event.gsensor.data[1], event.gsensor.data[2]);
                break;
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                if (event.gaxis.axis < num_axes)
                    axes[event.gaxis.axis] = event.gaxis.value;
                test_ui_log(&ui, "%s which=%d axis=%d (%s) value=%+d",
                            sdl_event_name(event.type), event.gaxis.which, event.gaxis.axis,
                            SDL_GetGamepadStringForAxis(event.gaxis.axis)
                                ? SDL_GetGamepadStringForAxis(event.gaxis.axis) : "?",
                            event.gaxis.value);
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                if (event.gbutton.button < num_buttons)
                    buttons[event.gbutton.button] = event.gbutton.down ? 1 : 0;
                test_ui_log(&ui, "%s which=%d button=%d (%s) %s",
                            sdl_event_name(event.type), event.gbutton.which, event.gbutton.button,
                            SDL_GetGamepadStringForButton(event.gbutton.button)
                                ? SDL_GetGamepadStringForButton(event.gbutton.button) : "?",
                            event.gbutton.down ? "DOWN" : "UP");
                if (event.gbutton.button == SDL_GAMEPAD_BUTTON_MISC1 &&
                    SDL_GetGamepadType(gamepad) == SDL_GAMEPAD_TYPE_PS5 &&
                    event.gbutton.down)
                {
                    CyclePS5TriggerEffect(gamepad);
                    test_ui_log(&ui, "PS5 trigger effect cycled");
                }
                break;
            case SDL_EVENT_GAMEPAD_ADDED:
                test_ui_log(&ui, "%s which=%d", sdl_event_name(event.type), event.gdevice.which);
                break;
            case SDL_EVENT_GAMEPAD_REMOVED:
                test_ui_log(&ui, "*** DISCONNECTED *** %s which=%d%s",
                            sdl_event_name(event.type), event.gdevice.which,
                            event.gdevice.which == open_iid ? " (this device)" : "");
                if (event.gdevice.which == open_iid)
                {
                    test_ui_log(&ui, "*** Controller unplugged — back to menu ***");
                    quit = 1;
                }
                break;
            case SDL_EVENT_JOYSTICK_AXIS_MOTION:
                test_ui_log(&ui, "%s joy=%d axis=%d value=%+d",
                            sdl_event_name(event.type),
                            event.jaxis.which, event.jaxis.axis, event.jaxis.value);
                break;
            case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
            case SDL_EVENT_JOYSTICK_BUTTON_UP:
                test_ui_log(&ui, "%s joy=%d btn=%d down=%d",
                            sdl_event_name(event.type),
                            event.jbutton.which, event.jbutton.button, event.jbutton.down);
                break;
            case SDL_EVENT_JOYSTICK_HAT_MOTION:
                test_ui_log(&ui, "%s joy=%d hat=%d value=%d",
                            sdl_event_name(event.type),
                            event.jhat.which, event.jhat.hat, event.jhat.value);
                break;
            case SDL_EVENT_JOYSTICK_BALL_MOTION:
                test_ui_log(&ui, "%s joy=%d ball=%d dx=%d dy=%d",
                            sdl_event_name(event.type),
                            event.jball.which, event.jball.ball,
                            event.jball.xrel, event.jball.yrel);
                break;
            case SDL_EVENT_JOYSTICK_ADDED:
                test_ui_log(&ui, "%s which=%d", sdl_event_name(event.type), event.jdevice.which);
                break;
            case SDL_EVENT_JOYSTICK_REMOVED:
                test_ui_log(&ui, "*** DISCONNECTED *** %s which=%d%s",
                            sdl_event_name(event.type), event.jdevice.which,
                            event.jdevice.which == open_iid ? " (this device)" : "");
                if (event.jdevice.which == open_iid)
                {
                    test_ui_log(&ui, "*** Device unplugged — back to menu ***");
                    quit = 1;
                }
                break;
            case SDL_EVENT_GAMEPAD_REMAPPED:
                test_ui_log(&ui, "%s which=%d", sdl_event_name(event.type), event.gdevice.which);
                break;
            case SDL_EVENT_CLIPBOARD_UPDATE:
            case SDL_EVENT_KEYMAP_CHANGED:
            case SDL_EVENT_POLL_SENTINEL:
                test_ui_log(&ui, "%s", sdl_event_name(event.type));
                break;
            case SDL_EVENT_QUIT:
                quit = 1;
                break;
            default:
                test_ui_log(&ui, "%s", sdl_event_name(event.type));
                break;
            }

            if (trigger_effect == 0)
            {
                Sint16 left = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
                Sint16 right = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
                SDL_RumbleGamepad(gamepad, ConvertAxisToRumble(left), ConvertAxisToRumble(right), 250);
                Sint16 ly = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
                Sint16 ry = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY);
                SDL_RumbleGamepadTriggers(gamepad, ConvertAxisToRumble(ly), ConvertAxisToRumble(ry), 250);
            }
        }

        if (1) /* always paint; avoids blank screen until first event */
        {
            {
                TestUIButtonGroup pad_groups[] = {
                    { 0, 4 },
                    { 4, 7 },
                    { 7, 11 },
                    { 11, 15 },
                    { 15, num_buttons },
                };
                int ng = 5;
                if (num_buttons <= 15)
                    ng = 4;
                test_ui_draw_joystick(&ui, SDL_GetGamepadName(gamepad),
                                      SDL_GetGamepadPlayerIndex(gamepad),
                                      axes, num_axes, buttons, num_buttons, labels,
                                      pad_groups, ng,
                                      NULL, 0, NULL, 0);
            }
            test_ui_refresh(&ui);
        }
        {
            TestUIAction act = test_ui_poll_action(&ui);
            if (act == TEST_UI_QUIT)
            {
                test_ui_shutdown(&ui);
                exit(EXIT_SUCCESS);
            }
            if (act == TEST_UI_BACK)
                quit = 1;
        }
    }

    test_ui_shutdown(&ui);
    free(labels);
    free(buttons);
    free(axes);
}

static void test_gamecontroller(int gamecontroller_idx)
{
    SDL_JoystickID id = joystick_id_at_index(gamecontroller_idx);
    SDL_Gamepad* gamepad = SDL_IsGamepad(id) ? SDL_OpenGamepad(id) : NULL;
    if (!gamepad)
    {
        printf("error: not a gamecontroller\n");
    }
    else
    {
        //test_gamecontroller_events(gamepad);
        test_gamecontroller_state(gamepad);

        SDL_CloseGamepad(gamepad);
    }
}

static void event_joystick(int joy_idx)
{
    SDL_JoystickID id = joystick_id_at_index(joy_idx);
    SDL_Joystick* joy = SDL_OpenJoystick(id);
    if (!joy)
    {
        fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
    }
    else
    {
        print_joystick_info(joy_idx, joy, NULL);

        printf("Entering joystick test loop, press Ctrl-c to exit\n");
        int quit = 0;
        SDL_Event event;

        while (!quit && SDL_WaitEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_JOYSTICK_AXIS_MOTION:
                printf("SDL_EVENT_JOYSTICK_AXIS_MOTION: joystick: %d axis: %d value: %d\n",
                       event.jaxis.which, event.jaxis.axis, event.jaxis.value);
                break;

            case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
                printf("SDL_EVENT_JOYSTICK_BUTTON_DOWN: joystick: %d button: %d state: %d\n",
                       event.jbutton.which, event.jbutton.button, event.jbutton.down);
                break;

            case SDL_EVENT_JOYSTICK_BUTTON_UP:
                printf("SDL_EVENT_JOYSTICK_BUTTON_UP: joystick: %d button: %d state: %d\n",
                       event.jbutton.which, event.jbutton.button, event.jbutton.down);
                break;

            case SDL_EVENT_JOYSTICK_HAT_MOTION:
                printf("SDL_EVENT_JOYSTICK_HAT_MOTION: joystick: %d hat: %d value: %d\n",
                       event.jhat.which, event.jhat.hat, event.jhat.value);
                break;

            case SDL_EVENT_JOYSTICK_BALL_MOTION:
                printf("SDL_EVENT_JOYSTICK_BALL_MOTION: joystick: %d ball: %d x: %d y: %d\n",
                       event.jball.which, event.jball.ball, event.jball.xrel, event.jball.yrel);
                break;

            case SDL_EVENT_JOYSTICK_ADDED:
                printf("SDL_EVENT_JOYSTICK_ADDED which:%d\n", event.jdevice.which);
                break;

            case SDL_EVENT_JOYSTICK_REMOVED:
                printf("SDL_EVENT_JOYSTICK_REMOVED which:%d\n", event.jdevice.which);
                break;

            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                printf("SDL_EVENT_GAMEPAD_BUTTON_DOWN\n");
                break;

            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                printf("SDL_EVENT_GAMEPAD_BUTTON_UP\n");
                break;

            case SDL_EVENT_GAMEPAD_ADDED:
                printf("SDL_EVENT_GAMEPAD_ADDED which:%d\n", event.gdevice.which);
                break;

            case SDL_EVENT_GAMEPAD_REMOVED:
                printf("SDL_EVENT_GAMEPAD_REMOVED which:%d\n", event.gdevice.which);
                break;

            case SDL_EVENT_GAMEPAD_REMAPPED:
                printf("SDL_EVENT_GAMEPAD_REMAPPED which:%d\n", event.gdevice.which);
                break;

            case SDL_EVENT_QUIT:
                quit = 1;
                printf("Recieved interrupt, exiting\n");
                break;

            default:
                printf("%s\n", sdl_event_name(event.type));
                break;
            }
        }
        SDL_CloseJoystick(joy);
    }
}

static void test_rumble(int joy_idx)
{
    SDL_JoystickID id = joystick_id_at_index(joy_idx);
    SDL_Joystick* joy = SDL_OpenJoystick(id);
    if (!joy)
    {
        fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
    }
    else
    {
        if (!SDL_RumbleJoystick(joy, 0xFFFF, 0xFFFF, 3000))
        {
            fprintf(stderr, "rumble failed on joystick %d: %s\n", joy_idx, SDL_GetError());
        }
        else
        {
            SDL_Delay(3000);
        }
        SDL_CloseJoystick(joy);
    }
}



static void gui_test_joystick(int joy_idx)
{
    SDL_JoystickID id = joystick_id_at_index(joy_idx);
    SDL_Joystick* joy = SDL_OpenJoystick(id);
    if (!joy) { fprintf(stderr, "Unable to open joystick %d\n", joy_idx); return; }
    int num_axes = SDL_GetNumJoystickAxes(joy);
    int num_buttons = SDL_GetNumJoystickButtons(joy);
    int num_hats = SDL_GetNumJoystickHats(joy);
    int num_balls = SDL_GetNumJoystickBalls(joy);
    if (num_axes < 0 || num_buttons < 0 || num_hats < 0 || num_balls < 0) {
        fprintf(stderr, "Unable to query joystick layout: %s\n", SDL_GetError());
        SDL_CloseJoystick(joy); return;
    }
    Sint16* axes = calloc((size_t)num_axes, sizeof(Sint16));
    Uint8* buttons = calloc((size_t)num_buttons, sizeof(Uint8));
    Uint8* hats = calloc((size_t)num_hats, sizeof(Uint8));
    Sint16* balls = calloc((size_t)num_balls, 2 * sizeof(Sint16));
    for (int i = 0; i < num_axes; ++i) axes[i] = SDL_GetJoystickAxis(joy, i);
    for (int i = 0; i < num_buttons; ++i) buttons[i] = SDL_GetJoystickButton(joy, i) ? 1 : 0;
    for (int i = 0; i < num_hats; ++i) hats[i] = SDL_GetJoystickHat(joy, i);
    SDL_JoystickID open_iid = id;

    GraphPresent3 gp;
    char title[128];
    snprintf(title, sizeof(title), "sdl3-jstest — %s",
             SDL_GetJoystickName(joy) ? SDL_GetJoystickName(joy) : "joystick");
    if (!graph_present3_open(&gp, title)) {
        free(balls); free(hats); free(buttons); free(axes); SDL_CloseJoystick(joy); return;
    }
    graph_ui_log(&gp.ui, "Opened joystick %d id=%u '%s'", joy_idx, (unsigned)id,
                 SDL_GetJoystickName(joy) ? SDL_GetJoystickName(joy) : "?");

    int quit = 0;
    SDL_Event event;
    while (!quit) {
        SDL_Delay(10);
        while (SDL_PollEvent(&event)) {
            TestUIAction act = TEST_UI_CONTINUE;
            if (graph_present3_map_key(&event, NULL, &act)) {
                if (act == TEST_UI_BACK || act == TEST_UI_QUIT) quit = 1;
                continue;
            }
            switch (event.type) {
            case SDL_EVENT_JOYSTICK_AXIS_MOTION:
                if (event.jaxis.axis < num_axes) axes[event.jaxis.axis] = event.jaxis.value;
                graph_ui_log(&gp.ui, "%s axis=%d value=%+d", sdl_event_name(event.type),
                    event.jaxis.axis, event.jaxis.value);
                break;
            case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
            case SDL_EVENT_JOYSTICK_BUTTON_UP:
                if (event.jbutton.button < num_buttons)
                    buttons[event.jbutton.button] = event.jbutton.down ? 1 : 0;
                graph_ui_log(&gp.ui, "%s button=%d %s", sdl_event_name(event.type),
                    event.jbutton.button, event.jbutton.down ? "DOWN" : "UP");
                break;
            case SDL_EVENT_JOYSTICK_HAT_MOTION:
                if (event.jhat.hat < num_hats) hats[event.jhat.hat] = event.jhat.value;
                graph_ui_log(&gp.ui, "HAT hat=%d value=%d", event.jhat.hat, event.jhat.value);
                break;
            case SDL_EVENT_JOYSTICK_BALL_MOTION:
                if (event.jball.ball < num_balls) {
                    balls[2*event.jball.ball]=event.jball.xrel;
                    balls[2*event.jball.ball+1]=event.jball.yrel;
                }
                break;
            case SDL_EVENT_JOYSTICK_REMOVED:
                if (event.jdevice.which == open_iid) quit = 1;
                break;
            case SDL_EVENT_QUIT:
                quit = 1;
                break;
            default: break;
            }
        }
        graph_ui_draw_joystick(&gp.ui, SDL_GetJoystickName(joy), joy_idx,
            axes, num_axes, buttons, num_buttons, NULL, NULL, 0,
            hats, num_hats, balls, num_balls);
        graph_present3_frame(&gp);
    }
    graph_present3_close(&gp);
    free(balls); free(hats); free(buttons); free(axes);
    SDL_CloseJoystick(joy);
}

static int gui_interactive_menu(void)
{
    for (;;) {
        int njoy = 0;
        SDL_JoystickID* ids = SDL_GetJoysticks(&njoy);
        char** labels = calloc((size_t)(njoy > 0 ? njoy : 1), sizeof(char*));
        for (int i = 0; i < njoy; ++i) {
            char buf[256];
            const char* name = SDL_GetJoystickNameForID(ids[i]);
            bool gc = SDL_IsGamepad(ids[i]);
            snprintf(buf, sizeof(buf), "%s  %s", gc ? "[pad]" : "[joy]", name ? name : "(unknown)");
            labels[i] = malloc(strlen(buf)+1);
            if (labels[i]) memcpy(labels[i], buf, strlen(buf)+1);
        }
        GraphPresent3 gp;
        if (!graph_present3_open(&gp, "sdl3-jstest — select device")) {
            for (int i = 0; i < njoy; ++i) free(labels[i]);
            free(labels); if (ids) SDL_free(ids); return 1;
        }
        gp.ui.menu_selected = 0;
        int sel = -1, done = 0;
        while (!done) {
            graph_ui_draw_menu(&gp.ui, "Select joystick / gamepad",
                               (const char* const*)labels, njoy);
            graph_present3_frame(&gp);
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                int mkey = 0;
                if (event.type == SDL_EVENT_QUIT) { sel = -1; done = 1; break; }
                if (graph_present3_map_key(&event, &mkey, NULL)) {
                    int r = graph_ui_menu_key(&gp.ui, mkey, njoy);
                    if (r == -1) { sel = -1; done = 1; }
                    else if (r >= 0) { sel = r; done = 1; }
                }
            }
            SDL_Delay(16);
        }
        graph_present3_close(&gp);
        for (int i = 0; i < njoy; ++i) free(labels[i]);
        free(labels);
        if (ids) SDL_free(ids);
        if (sel < 0) return 0;
        gui_test_joystick(sel);
    }
}

static int interactive_menu(void)
{
    for (;;)
    {
        int count = 0;
        SDL_JoystickID* ids = SDL_GetJoysticks(&count);
        char** labels = calloc((size_t)(count > 0 ? count : 1), sizeof(char*));
        int* is_gc = calloc((size_t)(count > 0 ? count : 1), sizeof(int));
        for (int i = 0; i < count; ++i)
        {
            char buf[256];
            const char* name = SDL_GetJoystickNameForID(ids[i]);
            int gc = SDL_IsGamepad(ids[i]);
            is_gc[i] = gc;
            snprintf(buf, sizeof(buf), "%s  %s",
                     gc ? "[pad]" : "[joy]",
                     name ? name : "(unknown)");
            labels[i] = malloc(strlen(buf) + 1);
            if (labels[i])
                memcpy(labels[i], buf, strlen(buf) + 1);
        }

        TestUI ui;
        test_ui_init(&ui);
        int sel = test_ui_menu_select(&ui, "Select joystick / gamepad",
                                      (const char* const*)labels, count);
        test_ui_shutdown(&ui);

        for (int i = 0; i < count; ++i)
            free(labels[i]);
        free(labels);
        int chosen_gc = (sel >= 0 && sel < count) ? is_gc[sel] : 0;
        free(is_gc);
        SDL_free(ids);

        if (sel < 0)
            return 0;

        if (chosen_gc)
            test_gamecontroller(sel);
        else
            test_joystick(sel);
    }
}

int main(int argc, char** argv)
{
    /* interactive menu when argc == 1, after SDL_Init */

    // SDL2 will only report events when the window has focus, so set
    // this hint as we don't have a window
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    // FIXME: We don't need video, but without it SDL will fail to work in SDL_WaitEvent()
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC))
    {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
    }
    else
    {
        atexit(SDL_Quit);

        {
            int ret = SDL_AddGamepadMappingsFromFile(SDL3_JSTEST_DATADIR "/gamecontrollerdb.txt");
            if (ret < 0)
            {
                ret = SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt");
            }

            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read gamecontrollerdb.txt: %s\n", SDL_GetError());
            }
        }

        if (argc == 1)
        {
            interactive_menu();
        }
        else if (argc == 2 && (strcmp(argv[1], "--help") == 0 ||
            strcmp(argv[1], "-h") == 0))
        {
            print_help(argv[0]);
        }
        else if (argc == 2 && (strcmp(argv[1], "--version") == 0))
        {
            printf("sdl3-jstest %s\n", SDL_JSTEST_VERSION);
            exit(EXIT_SUCCESS);
        }
        else if (argc == 2 && (strcmp(argv[1], "--list") == 0 ||
            (strcmp(argv[1], "-l") == 0)))
        {
            list_joysticks();
        }
        else if (argc == 3 && (strcmp(argv[1], "--gamecontroller") == 0 ||
            strcmp(argv[1], "-g") == 0))
        {
            int idx;
            if (!str2int(argv[2], &idx))
            {
                fprintf(stderr, "Error: JOYSTICKNUM argument must be a number, but was '%s'\n", argv[2]);
                exit(1);
            }
            else
            {
                test_gamecontroller(idx);
            }
        }
        else if (argc == 3 && (strcmp(argv[1], "--test") == 0 ||
            strcmp(argv[1], "-t") == 0))
        {
            int joy_idx;
            if (!str2int(argv[2], &joy_idx))
            {
                fprintf(stderr, "Error: JOYSTICKNUM argument must be a number, but was '%s'\n", argv[2]);
                exit(1);
            }
            else
            {
                test_joystick(joy_idx);
            }
        }
        else if (argc == 3 && (strcmp(argv[1], "--event") == 0 ||
            strcmp(argv[1], "-e") == 0))
        {
            int joy_idx;
            if (!str2int(argv[2], &joy_idx))
            {
                fprintf(stderr, "Error: JOYSTICKNUM argument must be a number, but was '%s'\n", argv[2]);
                exit(1);
            }
            event_joystick(joy_idx);
        }
        else if (argc == 3 && (strcmp(argv[1], "--rumble") == 0 ||
            strcmp(argv[1], "-r") == 0))
        {
            int idx;
            if (!str2int(argv[2], &idx))
            {
                fprintf(stderr, "Error: JOYSTICKNUM argument must be a number, but was '%s'\n", argv[2]);
                exit(1);
            }
            else
            {
                test_rumble(idx);
            }
        }
        else if ((argc == 2 || argc == 3) &&
                 (strcmp(argv[1], "--gui") == 0 || strcmp(argv[1], "-G") == 0))
        {
            if (argc == 2)
                gui_interactive_menu();
            else {
                int joy_idx;
                if (!str2int(argv[2], &joy_idx)) {
                    fprintf(stderr, "Error: JOYSTICKNUM argument must be a number, but was '%s'\n", argv[2]);
                    exit(1);
                }
                gui_test_joystick(joy_idx);
            }
        }
        else
        {
            fprintf(stderr, "%s: unknown arguments\n", argv[0]);
            fprintf(stderr, "Try '%s --help' for more informations\n", argv[0]);
        }
    }

    return EXIT_SUCCESS;
}

/* EOF */
