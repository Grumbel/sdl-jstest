// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <SDL3/SDL.h>
/* Android: main() becomes SDL_main, called from SDLActivity. */
#include <SDL3/SDL_main.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <stdint.h>
#include "common/cli.h"
#include "common/report.h"
#include "ui/graph_ui.h"
#include "ui/gui_nav.h"
#include "icons/sdl3_jstest_icon.h"
#if defined(__EMSCRIPTEN__)
#  include <emscripten.h>
#endif
#if defined(SDL_JSTEST_NCURSES)
#  include <curses.h>
#  define DUALSENSE_GAMEPAD_TYPE SDL_Gamepad
#  define DUALSENSE_SEND_EFFECT  SDL_SendGamepadEffect
#  include "utils/dualsense.h"
#  include "ui/test_ui.h"
#endif

/* Process-wide exit request (SIGINT/SIGTERM, window close, Ctrl-C in GUI). */
static volatile sig_atomic_t g_app_quit = 0;

static void app_quit_signal(int sig)
{
    (void)sig;
    g_app_quit = 1;
}

static void app_install_quit_handlers(void)
{
    signal(SIGINT, app_quit_signal);
    signal(SIGTERM, app_quit_signal);
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

#if defined(SDL_JSTEST_NCURSES)
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
#endif /* SDL_JSTEST_NCURSES */

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

/* Display name (gamepkgstd INT-2): window title, launcher, metainfo. */
#define DISPLAY_NAME "SDL3 Joystick Test"

/* ---- Window UI ----
   One window for the whole session (Android and KMSDRM have one surface):
   the 640x480 GraphUI buffer scaled to it with the aspect ratio kept. */

typedef struct Gui {
    SDL_Window* win;
    SDL_Renderer* ren;
    SDL_Texture* tex;
    GraphUI ui;
    bool vsync;
    /* Every gamepad stays open so that its buttons navigate. */
    SDL_Gamepad* pads[16];
    GuiHold hold;      /* gamepad Back held in a device view */
    GuiStick stick;    /* left stick in the menu */
} Gui;

static void report_window(Gui* gui)
{
    int w, h, pw, ph;
    SDL_GetWindowSize(gui->win, &w, &h);
    SDL_GetWindowSizeInPixels(gui->win, &pw, &ph);
    SDL_WindowFlags flags = SDL_GetWindowFlags(gui->win);
    SDL_DisplayID display = SDL_GetDisplayForWindow(gui->win);
    const SDL_DisplayMode* mode = display ? SDL_GetCurrentDisplayMode(display) : NULL;
    if (mode)
        report_line("video", "%s, display %u %dx%d @ %.2f Hz %s, scale %.2f, window %dx%d, pixels %dx%d%s",
                    SDL_GetCurrentVideoDriver(), (unsigned)display, mode->w, mode->h,
                    (double)mode->refresh_rate, SDL_GetPixelFormatName(mode->format),
                    (double)SDL_GetWindowDisplayScale(gui->win), w, h, pw, ph,
                    (flags & SDL_WINDOW_FULLSCREEN) ? ", fullscreen" : "");
    else
        report_line("video", "%s, window %dx%d, display mode unknown: %s",
                    SDL_GetCurrentVideoDriver(), w, h, SDL_GetError());

    SDL_PropertiesID props = SDL_GetRendererProperties(gui->ren);
    int vsync = 0;
    gui->vsync = SDL_GetRenderVSync(gui->ren, &vsync) && vsync != 0;
    report_line("renderer", "%s%s, max texture %lld", SDL_GetRendererName(gui->ren),
                gui->vsync ? ", vsync" : "",
                (long long)SDL_GetNumberProperty(props, SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0));

    /* The renderer's GL context is current after creating it. */
    if (SDL_GL_GetCurrentContext()) {
        typedef const unsigned char* (*GetStringFn)(unsigned int);
        GetStringFn get_string = (GetStringFn)SDL_GL_GetProcAddress("glGetString");
        if (get_string) {
            const char* vendor = (const char*)get_string(0x1F00);
            const char* renderer = (const char*)get_string(0x1F01);
            const char* version = (const char*)get_string(0x1F02);
            const char* glsl = (const char*)get_string(0x8B8C);
            report_line("opengl", "%s, %s, %s, GLSL %s", vendor ? vendor : "?",
                        renderer ? renderer : "?", version ? version : "?", glsl ? glsl : "?");
        }
    }
}

static void gui_open_pad(Gui* gui, SDL_JoystickID id)
{
    for (size_t i = 0; i < SDL_arraysize(gui->pads); ++i) {
        if (gui->pads[i] && SDL_GetGamepadID(gui->pads[i]) == id) return;
    }
    for (size_t i = 0; i < SDL_arraysize(gui->pads); ++i) {
        if (!gui->pads[i]) {
            gui->pads[i] = SDL_OpenGamepad(id);
            return;
        }
    }
}

static bool gui_open(Gui* gui, const JstestOptions* opts)
{
    memset(gui, 0, sizeof(*gui));
    if (!graph_ui_init(&gui->ui, GRAPH_UI_DEFAULT_W, GRAPH_UI_DEFAULT_H)) return false;

    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
    if (opts->fullscreen == 1) flags |= SDL_WINDOW_FULLSCREEN;
    int w = opts->width > 0 ? opts->width : gui->ui.w;
    int h = opts->height > 0 ? opts->height : gui->ui.h;
    gui->win = SDL_CreateWindow(DISPLAY_NAME, w, h, flags);
    if (!gui->win) {
        fprintf(stderr, "error: SDL_CreateWindow: %s\n", SDL_GetError());
        graph_ui_shutdown(&gui->ui);
        return false;
    }
    /* X11 only; Wayland takes the icon from the .desktop file (INT-9). */
    static uint32_t icon[SDL3_JSTEST_ICON_SIZE * SDL3_JSTEST_ICON_SIZE];
    memcpy(icon, sdl3_jstest_icon, sizeof(icon));
    SDL_Surface* icon_surface = SDL_CreateSurfaceFrom(SDL3_JSTEST_ICON_SIZE, SDL3_JSTEST_ICON_SIZE,
        SDL_PIXELFORMAT_ARGB8888, icon, SDL3_JSTEST_ICON_SIZE * 4);
    if (icon_surface) {
        SDL_SetWindowIcon(gui->win, icon_surface);
        SDL_DestroySurface(icon_surface);
    }
    gui->ren = SDL_CreateRenderer(gui->win, NULL);
    if (!gui->ren) {
        fprintf(stderr, "error: SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(gui->win);
        graph_ui_shutdown(&gui->ui);
        return false;
    }
    if (!SDL_SetRenderVSync(gui->ren, 1))
        fprintf(stderr, "renderer: no vsync (%s)\n", SDL_GetError());
    SDL_SetRenderLogicalPresentation(gui->ren, gui->ui.w, gui->ui.h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    gui->tex = SDL_CreateTexture(gui->ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                 gui->ui.w, gui->ui.h);
    if (!gui->tex) {
        fprintf(stderr, "error: SDL_CreateTexture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(gui->ren);
        SDL_DestroyWindow(gui->win);
        graph_ui_shutdown(&gui->ui);
        return false;
    }
    /* The bitmap font stays sharp when scaled. */
    SDL_SetTextureScaleMode(gui->tex, SDL_SCALEMODE_NEAREST);
    report_window(gui);

    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    for (int i = 0; i < count; ++i) gui_open_pad(gui, ids[i]);
    SDL_free(ids);
    return true;
}

static void gui_close(Gui* gui)
{
    for (size_t i = 0; i < SDL_arraysize(gui->pads); ++i)
        if (gui->pads[i]) SDL_CloseGamepad(gui->pads[i]);
    if (gui->tex) SDL_DestroyTexture(gui->tex);
    if (gui->ren) SDL_DestroyRenderer(gui->ren);
    if (gui->win) SDL_DestroyWindow(gui->win);
    graph_ui_shutdown(&gui->ui);
    memset(gui, 0, sizeof(*gui));
}

static void gui_present(Gui* gui)
{
    SDL_UpdateTexture(gui->tex, NULL, gui->ui.pixels, gui->ui.w * (int)sizeof(uint32_t));
    SDL_SetRenderDrawColor(gui->ren, 0, 0, 0, 255);
    SDL_RenderClear(gui->ren);
    SDL_RenderTexture(gui->ren, gui->tex, NULL, NULL);
    SDL_RenderPresent(gui->ren);
#if !defined(__EMSCRIPTEN__)
    /* The browser paces the frames itself. */
    if (!gui->vsync) SDL_Delay(10);
#endif
}

/* Keeps gui->pads in step with hotplugging. */
static void gui_track_pads(Gui* gui, const SDL_Event* ev)
{
    if (ev->type == SDL_EVENT_GAMEPAD_ADDED) {
        gui_open_pad(gui, ev->gdevice.which);
    } else if (ev->type == SDL_EVENT_GAMEPAD_REMOVED) {
        for (size_t i = 0; i < SDL_arraysize(gui->pads); ++i) {
            if (gui->pads[i] && SDL_GetGamepadID(gui->pads[i]) == ev->gdevice.which) {
                SDL_CloseGamepad(gui->pads[i]);
                gui->pads[i] = NULL;
            }
        }
    }
}

/* Event → navigation. In the menu, gamepads navigate (D-pad, left stick,
   A / Start, B / Back); in a device view only holding Back leaves, so that
   every button can be tested. */
static GuiAction gui_action(Gui* gui, SDL_Event* ev, bool in_menu, int* pick)
{
    switch (ev->type) {
    case SDL_EVENT_QUIT:
        return GUI_QUIT;
    case SDL_EVENT_KEY_DOWN: {
        SDL_Keycode k = ev->key.key;
        if (k == SDLK_C && (ev->key.mod & SDL_KMOD_CTRL)) return GUI_QUIT;
        if (k == SDLK_ESCAPE || k == SDLK_Q || k == SDLK_AC_BACK) return GUI_BACK;
        if (!in_menu) return GUI_NONE;
        if (k == SDLK_UP || k == SDLK_K) return GUI_UP;
        if (k == SDLK_DOWN || k == SDLK_J) return GUI_DOWN;
        if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) return GUI_CONFIRM;
        if (k >= SDLK_0 && k <= SDLK_9) {
            *pick = (int)(k - SDLK_0);
            return GUI_PICK;
        }
        return GUI_NONE;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        if (ev->button.button != SDL_BUTTON_LEFT) return GUI_NONE;
        /* Window → 640x480 buffer coordinates. */
        SDL_ConvertEventToRenderCoordinates(gui->ren, ev);
        int x = (int)ev->button.x, y = (int)ev->button.y;
        if (in_menu) {
            *pick = graph_ui_menu_hit(&gui->ui, x, y);
            return *pick >= 0 ? GUI_PICK : GUI_NONE;
        }
        return graph_ui_back_hit(&gui->ui, x, y) ? GUI_BACK : GUI_NONE;
    }
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP: {
        bool down = ev->gbutton.down;
        Uint8 b = ev->gbutton.button;
        if (!in_menu) {
            if (b == SDL_GAMEPAD_BUTTON_BACK) gui_hold_set(&gui->hold, down, (uint32_t)SDL_GetTicks());
            return GUI_NONE;
        }
        if (!down) return GUI_NONE;
        if (b == SDL_GAMEPAD_BUTTON_DPAD_UP) return GUI_UP;
        if (b == SDL_GAMEPAD_BUTTON_DPAD_DOWN) return GUI_DOWN;
        if (b == SDL_GAMEPAD_BUTTON_SOUTH || b == SDL_GAMEPAD_BUTTON_START) return GUI_CONFIRM;
        if (b == SDL_GAMEPAD_BUTTON_EAST || b == SDL_GAMEPAD_BUTTON_BACK) return GUI_BACK;
        return GUI_NONE;
    }
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (in_menu && ev->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY)
            return gui_stick_axis(&gui->stick, ev->gaxis.value);
        return GUI_NONE;
    default:
        return GUI_NONE;
    }
}

static const char* kViewHint = "Esc / Back key / hold Select: menu";

/* Per-frame part of a device view: the hold-to-leave check. */
static bool gui_view_hold_done(Gui* gui)
{
    gui->ui.hold_progress = gui_hold_progress(&gui->hold, (uint32_t)SDL_GetTicks(), GUI_HOLD_MS);
    return gui->ui.hold_progress >= 1.0f;
}

static void gui_view_begin(Gui* gui, const char* name)
{
    char title[160];
    snprintf(title, sizeof(title), DISPLAY_NAME " - %s", name ? name : "joystick");
    SDL_SetWindowTitle(gui->win, title);
    memset(&gui->hold, 0, sizeof(gui->hold));
    gui->ui.hint = kViewHint;
    gui->ui.hold_progress = 0.0f;
    gui->ui.log_len = gui->ui.log_head = 0;
}

/* The device view's state: the open joystick or gamepad and what it last
   reported. */
typedef struct GuiDevice {
    int idx;
    SDL_JoystickID id;
    SDL_Joystick* joy;   /* joystick view */
    SDL_Gamepad* pad;    /* gamepad view */
    int num_axes, num_buttons, num_hats, num_balls;
    Sint16* axes;
    Uint8* buttons;
    Uint8* hats;
    Sint16* balls;
    const char* labels[SDL_GAMEPAD_BUTTON_COUNT];
} GuiDevice;

static void gui_device_close(GuiDevice* dev)
{
    free(dev->balls); free(dev->hats); free(dev->buttons); free(dev->axes);
    if (dev->pad) SDL_CloseGamepad(dev->pad);
    if (dev->joy) SDL_CloseJoystick(dev->joy);
    memset(dev, 0, sizeof(*dev));
}

static bool gui_open_joystick(Gui* gui, GuiDevice* dev, int joy_idx)
{
    SDL_JoystickID id = joystick_id_at_index(joy_idx);
    SDL_Joystick* joy = id ? SDL_OpenJoystick(id) : NULL;
    if (!joy) {
        fprintf(stderr, "error: unable to open joystick %d: %s\n", joy_idx, SDL_GetError());
        return false;
    }

    int num_axes = SDL_GetNumJoystickAxes(joy);
    int num_buttons = SDL_GetNumJoystickButtons(joy);
    int num_hats = SDL_GetNumJoystickHats(joy);
    int num_balls = SDL_GetNumJoystickBalls(joy);
    if (num_axes < 0 || num_buttons < 0 || num_hats < 0 || num_balls < 0) {
        fprintf(stderr, "error: unable to query joystick layout: %s\n", SDL_GetError());
        SDL_CloseJoystick(joy);
        return false;
    }
    memset(dev, 0, sizeof(*dev));
    dev->idx = joy_idx;
    dev->id = id;
    dev->joy = joy;
    dev->num_axes = num_axes;
    dev->num_buttons = num_buttons;
    dev->num_hats = num_hats;
    dev->num_balls = num_balls;
    dev->axes = calloc((size_t)num_axes + 1, sizeof(Sint16));
    dev->buttons = calloc((size_t)num_buttons + 1, sizeof(Uint8));
    dev->hats = calloc((size_t)num_hats + 1, sizeof(Uint8));
    dev->balls = calloc((size_t)num_balls + 1, 2 * sizeof(Sint16));
    for (int i = 0; i < num_axes; ++i) dev->axes[i] = SDL_GetJoystickAxis(joy, i);
    for (int i = 0; i < num_buttons; ++i) dev->buttons[i] = SDL_GetJoystickButton(joy, i) ? 1 : 0;
    for (int i = 0; i < num_hats; ++i) dev->hats[i] = SDL_GetJoystickHat(joy, i);

    gui_view_begin(gui, SDL_GetJoystickName(joy));
    graph_ui_log(&gui->ui, "Opened joystick %d '%s' (id %u)", joy_idx,
                 SDL_GetJoystickName(joy) ? SDL_GetJoystickName(joy) : "?", (unsigned)id);
    return true;
}

static bool gui_open_gamepad(Gui* gui, GuiDevice* dev, int idx)
{
    SDL_JoystickID id = joystick_id_at_index(idx);
    SDL_Gamepad* pad = id ? SDL_OpenGamepad(id) : NULL;
    if (!pad) return gui_open_joystick(gui, dev, idx);

    memset(dev, 0, sizeof(*dev));
    dev->idx = idx;
    dev->id = id;
    dev->pad = pad;
    dev->num_axes = SDL_GAMEPAD_AXIS_COUNT;
    dev->num_buttons = SDL_GAMEPAD_BUTTON_COUNT;
    dev->axes = calloc(SDL_GAMEPAD_AXIS_COUNT, sizeof(Sint16));
    dev->buttons = calloc(SDL_GAMEPAD_BUTTON_COUNT, sizeof(Uint8));
    for (int i = 0; i < dev->num_buttons; ++i) {
        const char* s = SDL_GetGamepadStringForButton((SDL_GamepadButton)i);
        dev->labels[i] = s ? s : "";
        dev->buttons[i] = SDL_GetGamepadButton(pad, (SDL_GamepadButton)i) ? 1 : 0;
    }
    for (int i = 0; i < dev->num_axes; ++i)
        dev->axes[i] = SDL_GetGamepadAxis(pad, (SDL_GamepadAxis)i);

    gui_view_begin(gui, SDL_GetGamepadName(pad));
    graph_ui_log(&gui->ui, "Gamepad '%s' (id %u)",
                 SDL_GetGamepadName(pad) ? SDL_GetGamepadName(pad) : "?", (unsigned)id);
    return true;
}

static bool gui_open_device(Gui* gui, GuiDevice* dev, int idx)
{
    return SDL_IsGamepad(joystick_id_at_index(idx)) ? gui_open_gamepad(gui, dev, idx)
                                                    : gui_open_joystick(gui, dev, idx);
}

/* One joystick event of the device view; GUI_BACK when it left. */
static GuiAction gui_joystick_event(GuiDevice* dev, Gui* gui, const SDL_Event* event)
{
    switch (event->type) {
    case SDL_EVENT_JOYSTICK_AXIS_MOTION:
        if (event->jaxis.which != dev->id) break;
        if (event->jaxis.axis < dev->num_axes) dev->axes[event->jaxis.axis] = event->jaxis.value;
        graph_ui_log(&gui->ui, "%s axis=%d value=%+d", sdl_event_name(event->type),
            event->jaxis.axis, event->jaxis.value);
        break;
    case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
    case SDL_EVENT_JOYSTICK_BUTTON_UP:
        if (event->jbutton.which != dev->id) break;
        if (event->jbutton.button < dev->num_buttons)
            dev->buttons[event->jbutton.button] = event->jbutton.down ? 1 : 0;
        graph_ui_log(&gui->ui, "%s button=%d %s", sdl_event_name(event->type),
            event->jbutton.button, event->jbutton.down ? "DOWN" : "UP");
        break;
    case SDL_EVENT_JOYSTICK_HAT_MOTION:
        if (event->jhat.which != dev->id) break;
        if (event->jhat.hat < dev->num_hats) dev->hats[event->jhat.hat] = event->jhat.value;
        graph_ui_log(&gui->ui, "%s hat=%d value=0x%02x", sdl_event_name(event->type),
            event->jhat.hat, event->jhat.value);
        break;
    case SDL_EVENT_JOYSTICK_BALL_MOTION:
        if (event->jball.which != dev->id) break;
        if (event->jball.ball < dev->num_balls) {
            dev->balls[2*event->jball.ball] = event->jball.xrel;
            dev->balls[2*event->jball.ball+1] = event->jball.yrel;
        }
        graph_ui_log(&gui->ui, "%s ball=%d dx=%d dy=%d", sdl_event_name(event->type),
            event->jball.ball, event->jball.xrel, event->jball.yrel);
        break;
    case SDL_EVENT_JOYSTICK_REMOVED:
        if (event->jdevice.which == dev->id) {
            fprintf(stderr, "joystick %d: disconnected\n", dev->idx);
            return GUI_BACK;
        }
        break;
    default:
        break;
    }
    return GUI_NONE;
}

static GuiAction gui_gamepad_event(GuiDevice* dev, Gui* gui, const SDL_Event* event)
{
    switch (event->type) {
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (event->gaxis.which != dev->id) break;
        if (event->gaxis.axis < dev->num_axes) dev->axes[event->gaxis.axis] = event->gaxis.value;
        graph_ui_log(&gui->ui, "%s axis=%d (%s) value=%+d", sdl_event_name(event->type),
            event->gaxis.axis, SDL_GetGamepadStringForAxis((SDL_GamepadAxis)event->gaxis.axis),
            event->gaxis.value);
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        if (event->gbutton.which != dev->id) break;
        if (event->gbutton.button < dev->num_buttons)
            dev->buttons[event->gbutton.button] = event->gbutton.down ? 1 : 0;
        graph_ui_log(&gui->ui, "%s button=%d (%s) %s", sdl_event_name(event->type),
            event->gbutton.button,
            dev->labels[event->gbutton.button < dev->num_buttons ? event->gbutton.button : 0],
            event->gbutton.down ? "DOWN" : "UP");
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (event->gdevice.which == dev->id) {
            fprintf(stderr, "gamepad %d: disconnected\n", dev->idx);
            return GUI_BACK;
        }
        break;
    default:
        break;
    }
    return GUI_NONE;
}

/* One frame of the device view. */
static GuiAction gui_device_frame(Gui* gui, GuiDevice* dev)
{
    GuiAction result = GUI_NONE;
    SDL_Event event;
    while (result == GUI_NONE && SDL_PollEvent(&event)) {
        int pick = -1;
        gui_track_pads(gui, &event);
        GuiAction act = gui_action(gui, &event, false, &pick);
        if (act == GUI_BACK || act == GUI_QUIT) return act;
        result = dev->pad ? gui_gamepad_event(dev, gui, &event)
                          : gui_joystick_event(dev, gui, &event);
    }
    if (result == GUI_NONE && gui_view_hold_done(gui)) result = GUI_BACK;
    if (result != GUI_NONE) return result;

    if (dev->pad) {
        TestUIButtonGroup pad_groups[] = {{0,4},{4,7},{7,11},{11,15},{15,dev->num_buttons}};
        int ng = dev->num_buttons <= 15 ? 4 : 5;
        graph_ui_draw_joystick(&gui->ui, SDL_GetGamepadName(dev->pad), SDL_GetGamepadPlayerIndex(dev->pad),
            dev->axes, dev->num_axes, dev->buttons, dev->num_buttons, dev->labels, pad_groups, ng,
            NULL, 0, NULL, 0);
    } else {
        graph_ui_draw_joystick(&gui->ui, SDL_GetJoystickName(dev->joy), dev->idx,
            dev->axes, dev->num_axes, dev->buttons, dev->num_buttons, NULL, NULL, 0,
            dev->hats, dev->num_hats, dev->balls, dev->num_balls);
    }
    gui_present(gui);
    return GUI_NONE;
}

static void gui_menu_begin(Gui* gui)
{
    SDL_SetWindowTitle(gui->win, DISPLAY_NAME " - select device");
    gui->ui.hint = "Up/Down  Enter/A: open  Esc/B: quit";
    gui->ui.hold_progress = 0.0f;
    memset(&gui->stick, 0, sizeof(gui->stick));
}

/* One frame of the device menu, rebuilt every frame so that hotplugging
   shows at once; the last row quits (touch has no other way). Returns the
   chosen joystick index, -1 to quit or -2 to stay. */
static int gui_menu_frame(Gui* gui)
{
    int njoy = 0;
    SDL_JoystickID* ids = SDL_GetJoysticks(&njoy);
    int nitems = njoy + 1;
    char** labels = calloc((size_t)nitems, sizeof(char*));
    for (int i = 0; i < njoy; ++i) {
        char buf[256];
        const char* name = SDL_GetJoystickNameForID(ids[i]);
        snprintf(buf, sizeof(buf), "%s  %s", SDL_IsGamepad(ids[i]) ? "[pad]" : "[joy]",
                 name ? name : "(unknown)");
        labels[i] = SDL_strdup(buf);
    }
    SDL_free(ids);
    labels[njoy] = SDL_strdup(njoy ? "Quit" : "Quit   (no devices yet: connect one, it shows up here)");
    gui->ui.menu_numbered = njoy;

    int choice = -2;
    SDL_Event event;
    while (choice == -2 && SDL_PollEvent(&event)) {
        int pick = -1;
        gui_track_pads(gui, &event);
        switch (gui_action(gui, &event, true, &pick)) {
        case GUI_UP: graph_ui_menu_key(&gui->ui, GRAPH_UI_KEY_UP, nitems); break;
        case GUI_DOWN: graph_ui_menu_key(&gui->ui, GRAPH_UI_KEY_DOWN, nitems); break;
        case GUI_CONFIRM: choice = gui->ui.menu_selected; break;
        case GUI_PICK:
            if (pick >= 0 && pick < nitems) {
                gui->ui.menu_selected = pick;
                choice = pick;
            }
            break;
        case GUI_BACK: case GUI_QUIT: choice = -1; break;
        default: break;
        }
    }
    if (choice >= njoy) choice = -1;  /* "Quit", or a device that left */
    if (choice == -2) {
        graph_ui_draw_menu(&gui->ui, "Select joystick / gamepad",
                           (const char* const*)labels, nitems);
        gui_present(gui);
    }
    for (int i = 0; i < nitems; ++i) SDL_free(labels[i]);
    free(labels);
    return choice;
}

/* The window UI as one frame at a time (gamepkgstd WEB-1): the menu, or
   the view of an open device. */
typedef struct GuiSession {
    Gui gui;
    GuiDevice dev;
    bool in_device;
    bool single;       /* --gui JOYNUM: leaving the device view ends */
} GuiSession;

/* Returns false once the session is over. */
static bool gui_frame(GuiSession* s)
{
    if (g_app_quit) return false;
    if (!s->in_device) {
        int sel = gui_menu_frame(&s->gui);
        if (sel == -1) return false;
        if (sel >= 0) {
            if (gui_open_device(&s->gui, &s->dev, sel)) s->in_device = true;
            else gui_menu_begin(&s->gui);
        }
        return true;
    }
    GuiAction act = gui_device_frame(&s->gui, &s->dev);
    if (act == GUI_NONE) return true;
    gui_device_close(&s->dev);
    s->in_device = false;
    if (act == GUI_QUIT || s->single) return false;
    s->gui.ui.hint = NULL;
    gui_menu_begin(&s->gui);
    return true;
}

static void gui_end(GuiSession* s)
{
    if (s->in_device) gui_device_close(&s->dev);
    gui_close(&s->gui);
}

#if defined(__EMSCRIPTEN__)
static void gui_frame_web(void* arg)
{
    GuiSession* s = arg;
    if (!gui_frame(s)) {
        emscripten_cancel_main_loop();
        gui_end(s);
        SDL_Quit();
        /* The page says so (mk/wasm/shell.html). */
        emscripten_run_script("if (Module.onQuit) Module.onQuit();");
    }
}
#endif

/* --gui [JOYNUM], and the default without a terminal UI. With JOYNUM,
   leaving the device view ends the program. */
static int gui_run(const JstestOptions* opts)
{
    /* Static: in the browser it outlives main(). */
    static GuiSession s;
    memset(&s, 0, sizeof(s));
    if (!gui_open(&s.gui, opts)) return EXIT_FAILURE;
    if (opts->joy_idx >= 0) {
        if (!gui_open_device(&s.gui, &s.dev, opts->joy_idx)) {
            gui_close(&s.gui);
            return EXIT_SUCCESS;
        }
        s.in_device = true;
        s.single = true;
    } else {
        gui_menu_begin(&s.gui);
    }
#if defined(__EMSCRIPTEN__)
    /* Does not return: the browser calls gui_frame_web once per frame. */
    emscripten_set_main_loop_arg(gui_frame_web, &s, 0, true);
#else
    while (gui_frame(&s)) {}
    gui_end(&s);
#endif
    return EXIT_SUCCESS;
}

/* ---- Startup ---- */

/* gamecontrollerdb.txt, read through SDL (Android: APK asset) and kept for
   the report's mapping sources. */
typedef struct MappingDb {
    char path[4200];
    char* text;         /* SDL_free */
    int count;          /* mappings added, -1 on error */
    char error[256];
} MappingDb;

static void load_mapping_db(MappingDb* db, const DataDir* data_dir)
{
    memset(db, 0, sizeof(*db));
    db->count = -1;
    if (!platform_data_file(data_dir, "gamecontrollerdb.txt", db->path, sizeof(db->path))) {
        snprintf(db->error, sizeof(db->error), "no data directory");
        return;
    }
    size_t size = 0;
    db->text = SDL_LoadFile(db->path, &size);
    if (db->text)
        db->count = SDL_AddGamepadMappingsFromIO(SDL_IOFromConstMem(db->text, size), true);
    if (db->count < 0)
        snprintf(db->error, sizeof(db->error), "%s: %s", db->path, SDL_GetError());
}

static void report_startup(const DataDir* data_dir, const MappingDb* db)
{
    char lib[512];
    int linked = SDL_GetVersion();
    const char* mapped = report_mapped_library("libSDL3", lib, sizeof(lib));
    /* The revision is empty in builds from a plain source tree (Android). */
    const char* rev = SDL_GetRevision();
    report_line("sdl", "%d.%d.%d (built against %d.%d.%d)%s%s%s%s",
                SDL_VERSIONNUM_MAJOR(linked), SDL_VERSIONNUM_MINOR(linked), SDL_VERSIONNUM_MICRO(linked),
                SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION,
                *rev ? ", " : "", rev, mapped ? ", " : "", mapped ? mapped : "");
    report_ncurses();
    report_line("data", "%s (%s)", data_dir->path[0] ? data_dir->path : "-", data_dir->source);
    if (db->count >= 0)
        report_line("gamecontrollerdb", "%d mappings from %s", db->count, db->path);
    else
        report_line("gamecontrollerdb", "%s; using SDL's built-in mappings only", db->error);

    char drivers[256] = "";
    for (int i = 0; i < SDL_GetNumVideoDrivers(); ++i) {
        size_t len = strlen(drivers);
        snprintf(drivers + len, sizeof(drivers) - len, "%s%s", i ? " " : "", SDL_GetVideoDriver(i));
    }
    report_line("video drivers", "%s; current %s", drivers,
                SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "none");
    char renderers[256] = "";
    for (int i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
        size_t len = strlen(renderers);
        snprintf(renderers + len, sizeof(renderers) - len, "%s%s", i ? " " : "", SDL_GetRenderDriver(i));
    }
    report_line("render drivers", "%s", renderers);

    int njoy = 0, nhaptic = 0, ntouch = 0;
    SDL_JoystickID* ids = SDL_GetJoysticks(&njoy);
    SDL_free(SDL_GetHaptics(&nhaptic));
    SDL_free(SDL_GetTouchDevices(&ntouch));
    report_line("joysticks", "%d, %d haptic devices, %d touch devices", njoy, nhaptic, ntouch);
    for (int i = 0; i < njoy; ++i) {
        char guid[64];
        SDL_GUIDToString(SDL_GetJoystickGUIDForID(ids[i]), guid, sizeof(guid));
        char topic[32];
        snprintf(topic, sizeof(topic), "joystick %d", i);
        const char* name = SDL_GetJoystickNameForID(ids[i]);
        report_line(topic, "%s, GUID %s", name ? name : "?", guid);
        if (SDL_IsGamepad(ids[i])) {
            char* mapping = SDL_GetGamepadMappingForID(ids[i]);
            const char* bindings = mapping ? strchr(mapping, ',') : NULL;
            bindings = bindings ? strchr(bindings + 1, ',') : NULL;
            snprintf(topic, sizeof(topic), "joystick %d mapping", i);
            report_line(topic, "%s: %.120s%s",
                        report_mapping_source(mapping, db->text, "gamecontrollerdb.txt"),
                        bindings ? bindings + 1 : "?",
                        bindings && strlen(bindings + 1) > 120 ? "..." : "");
            SDL_free(mapping);
        }
    }
    SDL_free(ids);
}

/* Video is needed for the event queue, but not for the terminal modes: on a
   console without display those fall back to the dummy driver (LOG-6). */
static bool init_sdl(JstestMode mode)
{
    if (!SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC)) {
        fprintf(stderr, "error: unable to init SDL: %s\n", SDL_GetError());
        return false;
    }
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        if (mode == JSTEST_MODE_GUI) {
            fprintf(stderr, "error: unable to init SDL video: %s\n", SDL_GetError());
            return false;
        }
        fprintf(stderr, "video: unavailable (%s), using the dummy driver\n", SDL_GetError());
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
            fprintf(stderr, "error: unable to init SDL video: %s\n", SDL_GetError());
            return false;
        }
    }
    return true;
}

int main(int argc, char** argv)
{
    platform_attach_console();
    platform_log_to_logcat("sdl3-jstest");
    unsigned features = JSTEST_CLI_GAMECONTROLLER | JSTEST_CLI_RUMBLE;
    JstestOptions opts;
    if (!cli_parse(argc, argv, features, &opts)) {
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        return 2;
    }
#if defined(SDL_JSTEST_NCURSES)
    const bool has_terminal_ui = true;
#else
    const bool has_terminal_ui = false;
    if (opts.mode == JSTEST_MODE_DEFAULT) opts.mode = JSTEST_MODE_GUI;
    if (opts.mode == JSTEST_MODE_TEST || opts.mode == JSTEST_MODE_GAMECONTROLLER)
        opts.mode = JSTEST_MODE_GUI;
#endif

    DataDir data_dir;
    if (opts.mode == JSTEST_MODE_HELP) {
        cli_print_usage(argv[0], features, has_terminal_ui);
        return EXIT_SUCCESS;
    }
    if (opts.mode == JSTEST_MODE_VERSION) {
        printf("sdl3-jstest %s\n", SDL_JSTEST_VERSION);
        return EXIT_SUCCESS;
    }
    platform_find_data_dir(&data_dir, opts.data_dir, platform_exe_dir(), SDL3_JSTEST_DATADIR);
    if (opts.mode == JSTEST_MODE_SHOW_PATHS) {
        platform_print_paths(&data_dir);
        return EXIT_SUCCESS;
    }

    /* Android starts main() again in the same process. */
    g_app_quit = 0;

    report_program("sdl3-jstest", SDL_JSTEST_VERSION);
    report_system(SDL_GetPlatform(), SDL_GetNumLogicalCPUCores(), SDL_GetSystemRAM());
    report_env();

    // Without a focused window, SDL only reports joystick events with this hint.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    // Android: the Back key arrives as SDLK_AC_BACK instead of closing.
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    // The identifier is the X11 WM_CLASS and Wayland app_id (INT-3).
    SDL_SetAppMetadata(DISPLAY_NAME, SDL_JSTEST_VERSION, "sdl3-jstest");
    if (!init_sdl(opts.mode)) return EXIT_FAILURE;
    app_install_quit_handlers();

    MappingDb db;
    load_mapping_db(&db, &data_dir);
    report_startup(&data_dir, &db);

    int status = EXIT_SUCCESS;
    switch (opts.mode) {
    case JSTEST_MODE_LIST:
        list_joysticks();
        break;
    case JSTEST_MODE_GUI:
        status = gui_run(&opts);
        break;
    case JSTEST_MODE_EVENT:
        event_joystick(opts.joy_idx);
        break;
    case JSTEST_MODE_RUMBLE:
        test_rumble(opts.joy_idx);
        break;
#if defined(SDL_JSTEST_NCURSES)
    case JSTEST_MODE_DEFAULT:
        interactive_menu();
        break;
    case JSTEST_MODE_TEST:
        test_joystick(opts.joy_idx);
        break;
    case JSTEST_MODE_GAMECONTROLLER:
        test_gamecontroller(opts.joy_idx);
        break;
#endif
    default:
        break;
    }

    SDL_free(db.text);
    SDL_Quit();
    return status;
}

/* EOF */
