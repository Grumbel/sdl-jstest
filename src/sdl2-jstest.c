// SPDX-FileCopyrightText: 2014-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-FileCopyrightText: 2017 Jaime Moreira <jmoreirag@gmail.com>
// SPDX-FileCopyrightText: 2018 Anthony J. Bentley <anthony@anjbe.name>
// SPDX-FileCopyrightText: 2022 orbea <orbea@riseup.net>
// SPDX-FileCopyrightText: 2024 GhostlyDark <31742919+GhostlyDark@users.noreply.github.com>
// SPDX-FileCopyrightText: 2024-2025 ABeltramo <beltramo.ale@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <SDL.h>
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
#include "icons/sdl2_jstest_icon.h"
#if defined(SDL_JSTEST_NCURSES)
#  include "ui/test_ui.h"
#  include "utils/dualsense.h"
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
    E(SDL_QUIT);
    E(SDL_APP_TERMINATING);
    E(SDL_APP_LOWMEMORY);
    E(SDL_APP_WILLENTERBACKGROUND);
    E(SDL_APP_DIDENTERBACKGROUND);
    E(SDL_APP_WILLENTERFOREGROUND);
    E(SDL_APP_DIDENTERFOREGROUND);
    E(SDL_LOCALECHANGED);
    E(SDL_DISPLAYEVENT);
    E(SDL_WINDOWEVENT);
    E(SDL_SYSWMEVENT);
    E(SDL_KEYDOWN);
    E(SDL_KEYUP);
    E(SDL_TEXTEDITING);
    E(SDL_TEXTINPUT);
    E(SDL_KEYMAPCHANGED);
    E(SDL_MOUSEMOTION);
    E(SDL_MOUSEBUTTONDOWN);
    E(SDL_MOUSEBUTTONUP);
    E(SDL_MOUSEWHEEL);
    E(SDL_JOYAXISMOTION);
    E(SDL_JOYBALLMOTION);
    E(SDL_JOYHATMOTION);
    E(SDL_JOYBUTTONDOWN);
    E(SDL_JOYBUTTONUP);
    E(SDL_JOYDEVICEADDED);
    E(SDL_JOYDEVICEREMOVED);
    E(SDL_CONTROLLERAXISMOTION);
    E(SDL_CONTROLLERBUTTONDOWN);
    E(SDL_CONTROLLERBUTTONUP);
    E(SDL_CONTROLLERDEVICEADDED);
    E(SDL_CONTROLLERDEVICEREMOVED);
    E(SDL_CONTROLLERDEVICEREMAPPED);
    E(SDL_CONTROLLERTOUCHPADDOWN);
    E(SDL_CONTROLLERTOUCHPADMOTION);
    E(SDL_CONTROLLERTOUCHPADUP);
    E(SDL_CONTROLLERSENSORUPDATE);
    E(SDL_FINGERDOWN);
    E(SDL_FINGERUP);
    E(SDL_FINGERMOTION);
    E(SDL_DOLLARGESTURE);
    E(SDL_DOLLARRECORD);
    E(SDL_MULTIGESTURE);
    E(SDL_CLIPBOARDUPDATE);
    E(SDL_DROPFILE);
    E(SDL_DROPTEXT);
    E(SDL_DROPBEGIN);
    E(SDL_DROPCOMPLETE);
    E(SDL_AUDIODEVICEADDED);
    E(SDL_AUDIODEVICEREMOVED);
    E(SDL_SENSORUPDATE);
    E(SDL_RENDER_TARGETS_RESET);
    E(SDL_RENDER_DEVICE_RESET);
    E(SDL_POLLSENTINEL);
#undef E
    default:
    {
        static char buf[48];
        snprintf(buf, sizeof(buf), "UNKNOWN(%u)", (unsigned)type);
        return buf;
    }
    }
}


static void print_joystick_info(int joy_idx, SDL_Joystick* joy, SDL_GameController* gamepad)
{
    SDL_JoystickGUID guid = SDL_JoystickGetGUID(joy);
    char guid_str[1024];
    SDL_JoystickGetGUIDString(guid, guid_str, sizeof(guid_str));

    printf("Joystick Name:     '%s'\n", SDL_JoystickName(joy));
    printf("Joystick GUID:     %s\n", guid_str);
    printf("Joystick Number:   %2d\n", joy_idx);
    printf("Number of Axes:    %2d\n", SDL_JoystickNumAxes(joy));
    printf("Number of Buttons: %2d\n", SDL_JoystickNumButtons(joy));
    printf("Number of Hats:    %2d\n", SDL_JoystickNumHats(joy));
    printf("Number of Balls:   %2d\n", SDL_JoystickNumBalls(joy));
    printf("GameControllerConfig:\n");
    if (!gamepad)
    {
        printf("  missing (see 'gamecontrollerdb.txt' or SDL_GAMECONTROLLERCONFIG)\n");
    }
    else
    {
        printf("  Name:    '%s'\n", SDL_GameControllerName(gamepad));
        printf("  Mapping: '%s'\n", SDL_GameControllerMappingForGUID(guid));
    }
    printf("\n");
}

static void list_joysticks(void)
{
    int num_joysticks = SDL_NumJoysticks();
    if (num_joysticks == 0)
    {
        printf("No joysticks were found\n");
    }
    else
    {
        printf("Found %d joystick(s)\n\n", num_joysticks);
        for (int joy_idx = 0; joy_idx < num_joysticks; ++joy_idx)
        {
            SDL_Joystick* joy = SDL_JoystickOpen(joy_idx);
            if (!joy)
            {
                fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
            }
            else
            {
                SDL_GameController* gamepad = SDL_GameControllerOpen(joy_idx);
                print_joystick_info(joy_idx, joy, gamepad);
                if (gamepad)
                {
                    SDL_GameControllerClose(gamepad);
                }
                SDL_JoystickClose(joy);
            }
        }
    }
}

#if defined(SDL_JSTEST_NCURSES)
static void test_joystick(int joy_idx)
{
    SDL_Joystick* joy = SDL_JoystickOpen(joy_idx);
    if (!joy)
    {
        fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
        return;
    }

    int num_axes = SDL_JoystickNumAxes(joy);
    int num_buttons = SDL_JoystickNumButtons(joy);
    int num_hats = SDL_JoystickNumHats(joy);
    int num_balls = SDL_JoystickNumBalls(joy);
    if (num_axes < 0 || num_buttons < 0 || num_hats < 0 || num_balls < 0)
    {
        fprintf(stderr, "Unable to query joystick layout: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    Sint16* axes = calloc((size_t)num_axes, sizeof(Sint16));
    Uint8* buttons = calloc((size_t)num_buttons, sizeof(Uint8));
    Uint8* hats = calloc((size_t)num_hats, sizeof(Uint8));
    Sint16* balls = calloc((size_t)num_balls, 2 * sizeof(Sint16));

    SDL_JoystickID open_iid = SDL_JoystickInstanceID(joy);

    /* Seed current axis/button/hat state for the first paint */
    for (int i = 0; i < num_axes; ++i)
        axes[i] = SDL_JoystickGetAxis(joy, i);
    for (int i = 0; i < num_buttons; ++i)
        buttons[i] = SDL_JoystickGetButton(joy, i);
    for (int i = 0; i < num_hats; ++i)
        hats[i] = SDL_JoystickGetHat(joy, i);

    TestUI ui;
    test_ui_init(&ui);
    test_ui_log(&ui, "Opened joystick %d '%s' (instance %d)", joy_idx,
                SDL_JoystickName(joy) ? SDL_JoystickName(joy) : "?",
                (int)open_iid);

    int quit = 0;
    SDL_Event event;
    while (!quit)
    {
        SDL_Delay(10);
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_JOYAXISMOTION:
                if (event.jaxis.axis < num_axes)
                    axes[event.jaxis.axis] = event.jaxis.value;
                test_ui_log(&ui, "%s which=%d axis=%d value=%+d",
                            sdl_event_name(event.type),
                            event.jaxis.which, event.jaxis.axis, event.jaxis.value);
                break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                if (event.jbutton.button < num_buttons)
                    buttons[event.jbutton.button] = event.jbutton.state;
                test_ui_log(&ui, "%s which=%d button=%d %s",
                            sdl_event_name(event.type),
                            event.jbutton.which, event.jbutton.button,
                            event.jbutton.state ? "DOWN" : "UP");
                break;
            case SDL_JOYHATMOTION:
                if (event.jhat.hat < num_hats)
                    hats[event.jhat.hat] = event.jhat.value;
                test_ui_log(&ui, "%s which=%d hat=%d value=0x%02x",
                            sdl_event_name(event.type),
                            event.jhat.which, event.jhat.hat, event.jhat.value);
                break;
            case SDL_JOYBALLMOTION:
                if (event.jball.ball < num_balls)
                {
                    balls[2 * event.jball.ball] = event.jball.xrel;
                    balls[2 * event.jball.ball + 1] = event.jball.yrel;
                }
                test_ui_log(&ui, "%s which=%d ball=%d dx=%d dy=%d",
                            sdl_event_name(event.type),
                            event.jball.which, event.jball.ball,
                            event.jball.xrel, event.jball.yrel);
                break;
            case SDL_JOYDEVICEADDED:
                test_ui_log(&ui, "DEVICE ADDED   which=%d", event.jdevice.which);
                break;
            case SDL_JOYDEVICEREMOVED:
                test_ui_log(&ui, "*** DISCONNECTED *** %s which=%d%s",
                            sdl_event_name(event.type), event.jdevice.which,
                            event.jdevice.which == open_iid ? " (this device)" : "");
                if (event.jdevice.which == open_iid)
                {
                    test_ui_log(&ui, "*** Device unplugged — back to menu ***");
                    quit = 1;
                }
                break;
            case SDL_CONTROLLERDEVICEADDED:
                test_ui_log(&ui, "%s which=%d", sdl_event_name(event.type), event.cdevice.which);
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                test_ui_log(&ui, "%s which=%d", sdl_event_name(event.type), event.cdevice.which);
                break;
            case SDL_CONTROLLERDEVICEREMAPPED:
                test_ui_log(&ui, "%s which=%d", sdl_event_name(event.type), event.cdevice.which);
                break;
            case SDL_CLIPBOARDUPDATE:
            case SDL_KEYMAPCHANGED:
            case SDL_POLLSENTINEL:
                test_ui_log(&ui, "%s", sdl_event_name(event.type));
                break;
            case SDL_QUIT:
                quit = 1;
                break;
            default:
                test_ui_log(&ui, "%s", sdl_event_name(event.type));
                break;
            }
        }
        if (1) /* always paint; avoids blank screen until first event */
        {
            test_ui_draw_joystick(&ui, SDL_JoystickName(joy), joy_idx,
                                  axes, num_axes, buttons, num_buttons, NULL,
                                  NULL, 0,
                                  hats, num_hats, balls, num_balls);
            test_ui_refresh(&ui);
        }
        /*replaced*/if (0)
            quit = 1;
    }

    test_ui_shutdown(&ui);
    free(balls);
    free(hats);
    free(buttons);
    free(axes);
    SDL_JoystickClose(joy);
}

static Uint16 ConvertAxisToRumble(Sint16 axisval)
{
    /* Only start rumbling if the axis is past the halfway point */
    const Sint16 half_axis = (Sint16)SDL_ceil(SDL_JOYSTICK_AXIS_MAX / 2.0f);
    if (axisval > half_axis)
    {
        return (Uint16)(axisval - half_axis) * 4;
    }
    else
    {
        return 0;
    }
}

static void test_gamecontroller_state(SDL_GameController* gamepad)
{
    assert(gamepad);

    int num_axes = SDL_CONTROLLER_AXIS_MAX;
    int num_buttons = SDL_CONTROLLER_BUTTON_MAX;
    Sint16* axes = calloc((size_t)num_axes, sizeof(Sint16));
    Uint8* buttons = calloc((size_t)num_buttons, sizeof(Uint8));
    const char** labels = calloc((size_t)num_buttons, sizeof(char*));
    for (int i = 0; i < num_buttons; ++i)
    {
        const char* s = SDL_GameControllerGetStringForButton(i);
        labels[i] = s ? s : "";
    }

    bool has_gyro = SDL_GameControllerHasSensor(gamepad, SDL_SENSOR_GYRO);
    if (has_gyro)
        SDL_GameControllerSetSensorEnabled(gamepad, SDL_SENSOR_GYRO, SDL_TRUE);
    bool has_accel = SDL_GameControllerHasSensor(gamepad, SDL_SENSOR_ACCEL);
    if (has_accel)
        SDL_GameControllerSetSensorEnabled(gamepad, SDL_SENSOR_ACCEL, SDL_TRUE);

    /* Seed current state so the first frame is meaningful */
    for (int i = 0; i < num_axes; ++i)
        axes[i] = SDL_GameControllerGetAxis(gamepad, i);
    for (int i = 0; i < num_buttons; ++i)
        buttons[i] = SDL_GameControllerGetButton(gamepad, i) ? 1 : 0;

    SDL_Joystick* joy_ref = SDL_GameControllerGetJoystick(gamepad);
    SDL_JoystickID open_iid = joy_ref ? SDL_JoystickInstanceID(joy_ref) : -1;

    TestUI ui;
    test_ui_init(&ui);
    test_ui_log(&ui, "GameController '%s' (instance %d)",
                SDL_GameControllerName(gamepad), (int)open_iid);

    int quit = 0;
    SDL_Event event;
    while (!quit)
    {
        SDL_Delay(10);
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_CONTROLLERSENSORUPDATE:
                test_ui_log(&ui, "%s sensor=%d data=[%.3f %.3f %.3f]",
                            sdl_event_name(event.type), event.csensor.sensor,
                            event.csensor.data[0], event.csensor.data[1], event.csensor.data[2]);
                break;
            case SDL_CONTROLLERAXISMOTION:
                if (event.caxis.axis < num_axes)
                    axes[event.caxis.axis] = event.caxis.value;
                test_ui_log(&ui, "%s which=%d axis=%d (%s) value=%+d",
                            sdl_event_name(event.type), event.caxis.which, event.caxis.axis,
                            SDL_GameControllerGetStringForAxis(event.caxis.axis)
                                ? SDL_GameControllerGetStringForAxis(event.caxis.axis) : "?",
                            event.caxis.value);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                if (event.cbutton.button < num_buttons)
                    buttons[event.cbutton.button] = event.cbutton.state;
                test_ui_log(&ui, "%s which=%d button=%d (%s) %s",
                            sdl_event_name(event.type), event.cbutton.which, event.cbutton.button,
                            SDL_GameControllerGetStringForButton(event.cbutton.button)
                                ? SDL_GameControllerGetStringForButton(event.cbutton.button) : "?",
                            event.cbutton.state ? "DOWN" : "UP");
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_MISC1 &&
                    SDL_GameControllerGetType(gamepad) == SDL_CONTROLLER_TYPE_PS5 &&
                    event.cbutton.state)
                {
                    CyclePS5TriggerEffect(gamepad);
                    test_ui_log(&ui, "PS5 trigger effect cycled");
                }
                break;
            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMAPPED:
                test_ui_log(&ui, "%s which=%d", sdl_event_name(event.type), event.cdevice.which);
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                test_ui_log(&ui, "*** DISCONNECTED *** %s which=%d%s",
                            sdl_event_name(event.type), event.cdevice.which,
                            event.cdevice.which == open_iid ? " (this device)" : "");
                if (event.cdevice.which == open_iid)
                {
                    test_ui_log(&ui, "*** Controller unplugged — back to menu ***");
                    quit = 1;
                }
                break;
            case SDL_JOYAXISMOTION:
                test_ui_log(&ui, "%s which=%d axis=%d value=%+d",
                            sdl_event_name(event.type),
                            event.jaxis.which, event.jaxis.axis, event.jaxis.value);
                break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                test_ui_log(&ui, "%s which=%d button=%d state=%d",
                            sdl_event_name(event.type),
                            event.jbutton.which, event.jbutton.button, event.jbutton.state);
                break;
            case SDL_JOYHATMOTION:
                test_ui_log(&ui, "%s which=%d hat=%d value=0x%02x",
                            sdl_event_name(event.type),
                            event.jhat.which, event.jhat.hat, event.jhat.value);
                break;
            case SDL_JOYBALLMOTION:
                test_ui_log(&ui, "%s which=%d ball=%d dx=%d dy=%d",
                            sdl_event_name(event.type),
                            event.jball.which, event.jball.ball,
                            event.jball.xrel, event.jball.yrel);
                break;
            case SDL_JOYDEVICEADDED:
                test_ui_log(&ui, "%s which=%d", sdl_event_name(event.type), event.jdevice.which);
                break;
            case SDL_JOYDEVICEREMOVED:
                test_ui_log(&ui, "*** DISCONNECTED *** %s which=%d%s",
                            sdl_event_name(event.type), event.jdevice.which,
                            event.jdevice.which == open_iid ? " (this device)" : "");
                if (event.jdevice.which == open_iid)
                {
                    test_ui_log(&ui, "*** Device unplugged — back to menu ***");
                    quit = 1;
                }
                break;
            case SDL_CLIPBOARDUPDATE:
            case SDL_KEYMAPCHANGED:
            case SDL_POLLSENTINEL:
                test_ui_log(&ui, "%s", sdl_event_name(event.type));
                break;
            case SDL_QUIT:
                quit = 1;
                break;
            default:
                test_ui_log(&ui, "%s", sdl_event_name(event.type));
                break;
            }
            if (trigger_effect == 0)
            {
                Sint16 left = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
                Sint16 right = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
                SDL_GameControllerRumble(gamepad, ConvertAxisToRumble(left), ConvertAxisToRumble(right), 250);
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
                test_ui_draw_joystick(&ui, SDL_GameControllerName(gamepad),
                                      SDL_GameControllerGetPlayerIndex(gamepad),
                                      axes, num_axes, buttons, num_buttons, labels,
                                      pad_groups, ng,
                                      NULL, 0, NULL, 0);
            }
            test_ui_refresh(&ui);
        }
        /*replaced*/if (0)
            quit = 1;
    }
    test_ui_shutdown(&ui);
    free(labels);
    free(buttons);
    free(axes);
}

static void test_gamecontroller(int gamecontroller_idx)
{
    SDL_GameController* gamepad = SDL_GameControllerOpen(gamecontroller_idx);
    if (!gamepad)
    {
        printf("error: not a gamecontroller\n");
    }
    else
    {
        //test_gamecontroller_events(gamepad);
        test_gamecontroller_state(gamepad);

        SDL_GameControllerClose(gamepad);
    }
}

static int interactive_menu(void)
{
    for (;;)
    {
        int njoy = SDL_NumJoysticks();
        char** labels = calloc((size_t)(njoy > 0 ? njoy : 1), sizeof(char*));
        int* is_gc = calloc((size_t)(njoy > 0 ? njoy : 1), sizeof(int));
        for (int i = 0; i < njoy; ++i)
        {
            char buf[256];
            const char* name = SDL_JoystickNameForIndex(i);
            int gc = SDL_IsGameController(i);
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
        int sel = test_ui_menu_select(&ui, "Select joystick / gamecontroller",
                                      (const char* const*)labels, njoy);
        test_ui_shutdown(&ui);

        for (int i = 0; i < njoy; ++i)
            free(labels[i]);
        free(labels);
        free(is_gc);

        if (sel < 0)
            return 0;

        if (SDL_IsGameController(sel))
            test_gamecontroller(sel);
        else
            test_joystick(sel);
        /* Esc from test returns here and reopens menu */
    }
}
#endif /* SDL_JSTEST_NCURSES */

static void event_joystick(int joy_idx)
{
    SDL_Joystick* joy = SDL_JoystickOpen(joy_idx);
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
            case SDL_JOYAXISMOTION:
                printf("SDL_JOYAXISMOTION: joystick: %d axis: %d value: %d\n",
                       event.jaxis.which, event.jaxis.axis, event.jaxis.value);
                break;

            case SDL_JOYBUTTONDOWN:
                printf("SDL_JOYBUTTONDOWN: joystick: %d button: %d state: %d\n",
                       event.jbutton.which, event.jbutton.button, event.jbutton.state);
                break;

            case SDL_JOYBUTTONUP:
                printf("SDL_JOYBUTTONUP: joystick: %d button: %d state: %d\n",
                       event.jbutton.which, event.jbutton.button, event.jbutton.state);
                break;

            case SDL_JOYHATMOTION:
                printf("SDL_JOYHATMOTION: joystick: %d hat: %d value: %d\n",
                       event.jhat.which, event.jhat.hat, event.jhat.value);
                break;

            case SDL_JOYBALLMOTION:
                printf("SDL_JOYBALLMOTION: joystick: %d ball: %d x: %d y: %d\n",
                       event.jball.which, event.jball.ball, event.jball.xrel, event.jball.yrel);
                break;

            case SDL_JOYDEVICEADDED:
                printf("SDL_JOYDEVICEADDED which:%d\n", event.jdevice.which);
                break;

            case SDL_JOYDEVICEREMOVED:
                printf("SDL_JOYDEVICEREMOVED which:%d\n", event.jdevice.which);
                break;

            case SDL_CONTROLLERBUTTONDOWN:
                printf("SDL_CONTROLLERBUTTONDOWN\n");
                break;

            case SDL_CONTROLLERBUTTONUP:
                printf("SDL_CONTROLLERBUTTONUP\n");
                break;

            case SDL_CONTROLLERDEVICEADDED:
                printf("SDL_CONTROLLERDEVICEADDED which:%d\n", event.cdevice.which);
                break;

            case SDL_CONTROLLERDEVICEREMOVED:
                printf("SDL_CONTROLLERDEVICEREMOVED which:%d\n", event.cdevice.which);
                break;

            case SDL_CONTROLLERDEVICEREMAPPED:
                printf("SDL_CONTROLLERDEVICEREMAPPED which:%d\n", event.cdevice.which);
                break;

            case SDL_QUIT:
                quit = 1;
                printf("Recieved interrupt, exiting\n");
                break;

            default:
                printf("%s\n", sdl_event_name(event.type));
                break;
            }
        }
        SDL_JoystickClose(joy);
    }
}

static void test_rumble(int joy_idx)
{
    SDL_Joystick* joy = SDL_JoystickOpen(joy_idx);
    if (!joy)
    {
        fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
    }
    else
    {
#if SDL_VERSION_ATLEAST(2, 0, 18)
        if (SDL_JoystickHasRumble(joy) == SDL_FALSE)
        {
            fprintf(stderr, "rumble not supported on joystick %d\n", joy_idx);
        }
        else
        {
            SDL_GameControllerUpdate();
            SDL_JoystickRumble(joy, 0xFFFF, 0xFFFF, 3000);
            SDL_Delay(3000);
        }
#else /* SDL_VERSION_ATLEAST(2,0,18) */
    SDL_Haptic* haptic = SDL_HapticOpenFromJoystick(joy);
    if (!haptic)
    {
      fprintf(stderr, "Unable to open haptic on joystick %d\n", joy_idx);
      fprintf(stderr, "SDL_Error: %s\n", SDL_GetError());
    }
    else
    {
      if (!SDL_HapticRumbleSupported(haptic))
      {
        fprintf(stderr, "rumble not supported on joystick %d\n", joy_idx);
      }
      else
      {
        if (SDL_HapticRumbleInit(haptic) != 0)
        {
          fprintf(stderr, "failed to init rumble\n");
        }
        else
        {
          SDL_HapticRumblePlay(haptic, 1.0, 3000);
          SDL_Delay(3000);
        }
      }
      SDL_HapticClose(haptic);
    }
#endif /* SDL_VERSION_ATLEAST(2,0,18) */
        SDL_JoystickClose(joy);
    }
}



/* Display name (gamepkgstd INT-2): window title, launcher, metainfo. */
#define DISPLAY_NAME "SDL2 Joystick Test"

/* ---- Window UI ----
   One window for the whole session (Android and KMSDRM have one surface):
   the 640x480 GraphUI buffer scaled to it with the aspect ratio kept. */

typedef struct Gui {
    SDL_Window* win;
    SDL_Renderer* ren;
    SDL_Texture* tex;
    GraphUI ui;
    bool vsync;
    /* Every gamecontroller stays open so that its buttons navigate. */
    SDL_GameController* pads[16];
    GuiHold hold;      /* gamepad Back held in a device view */
    GuiStick stick;    /* left stick in the menu */
} Gui;

static void report_window(Gui* gui)
{
    int w, h, dw, dh;
    SDL_GetWindowSize(gui->win, &w, &h);
    SDL_GL_GetDrawableSize(gui->win, &dw, &dh);
    Uint32 flags = SDL_GetWindowFlags(gui->win);
    SDL_DisplayMode mode;
    int display = SDL_GetWindowDisplayIndex(gui->win);
    if (display >= 0 && SDL_GetCurrentDisplayMode(display, &mode) == 0)
        report_line("video", "%s, display %d %dx%d @ %d Hz %s, window %dx%d, drawable %dx%d%s",
                    SDL_GetCurrentVideoDriver(), display, mode.w, mode.h, mode.refresh_rate,
                    SDL_GetPixelFormatName(mode.format), w, h, dw, dh,
                    (flags & SDL_WINDOW_FULLSCREEN) ? ", fullscreen" : "");
    else
        report_line("video", "%s, window %dx%d, display mode unknown: %s",
                    SDL_GetCurrentVideoDriver(), w, h, SDL_GetError());

    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(gui->ren, &info) == 0) {
        report_line("renderer", "%s, %s%s%s, max texture %dx%d", info.name,
                    (info.flags & SDL_RENDERER_ACCELERATED) ? "accelerated" : "software",
                    (info.flags & SDL_RENDERER_PRESENTVSYNC) ? ", vsync" : "",
                    (info.flags & SDL_RENDERER_TARGETTEXTURE) ? ", target textures" : "",
                    info.max_texture_width, info.max_texture_height);
        gui->vsync = (info.flags & SDL_RENDERER_PRESENTVSYNC) != 0;
    }

    /* The renderer's GL context is current after creating it. */
    if (SDL_GL_GetCurrentContext()) {
        typedef const unsigned char* (*GetStringFn)(unsigned int);
        /* ISO C has no object → function pointer cast; copy the bits. */
        void* proc = SDL_GL_GetProcAddress("glGetString");
        GetStringFn get_string = NULL;
        memcpy(&get_string, &proc, sizeof(get_string));
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

static bool gui_open(Gui* gui, const JstestOptions* opts)
{
    memset(gui, 0, sizeof(*gui));
    if (!graph_ui_init(&gui->ui, GRAPH_UI_DEFAULT_W, GRAPH_UI_DEFAULT_H)) return false;

    /* The bitmap font stays sharp when scaled. */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    Uint32 flags = SDL_WINDOW_RESIZABLE;
    if (opts->fullscreen == 1) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    int w = opts->width > 0 ? opts->width : gui->ui.w;
    int h = opts->height > 0 ? opts->height : gui->ui.h;
    gui->win = SDL_CreateWindow(DISPLAY_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                w, h, flags);
    if (!gui->win) {
        fprintf(stderr, "error: SDL_CreateWindow: %s\n", SDL_GetError());
        graph_ui_shutdown(&gui->ui);
        return false;
    }
    /* X11 only; Wayland takes the icon from the .desktop file (INT-9). */
    static uint32_t icon[SDL2_JSTEST_ICON_SIZE * SDL2_JSTEST_ICON_SIZE];
    memcpy(icon, sdl2_jstest_icon, sizeof(icon));
    SDL_Surface* icon_surface = SDL_CreateRGBSurfaceFrom(icon, SDL2_JSTEST_ICON_SIZE, SDL2_JSTEST_ICON_SIZE,
        32, SDL2_JSTEST_ICON_SIZE * 4, 0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0xFF000000u);
    if (icon_surface) {
        SDL_SetWindowIcon(gui->win, icon_surface);
        SDL_FreeSurface(icon_surface);
    }
    gui->ren = SDL_CreateRenderer(gui->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!gui->ren) {
        fprintf(stderr, "renderer: no accelerated renderer (%s), trying any\n", SDL_GetError());
        gui->ren = SDL_CreateRenderer(gui->win, -1, 0);
    }
    if (!gui->ren) {
        fprintf(stderr, "error: SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(gui->win);
        graph_ui_shutdown(&gui->ui);
        return false;
    }
    SDL_RenderSetLogicalSize(gui->ren, gui->ui.w, gui->ui.h);
    gui->tex = SDL_CreateTexture(gui->ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                 gui->ui.w, gui->ui.h);
    if (!gui->tex) {
        fprintf(stderr, "error: SDL_CreateTexture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(gui->ren);
        SDL_DestroyWindow(gui->win);
        graph_ui_shutdown(&gui->ui);
        return false;
    }
    report_window(gui);

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i) && i < (int)SDL_arraysize(gui->pads))
            gui->pads[i] = SDL_GameControllerOpen(i);
    }
    return true;
}

static void gui_close(Gui* gui)
{
    for (size_t i = 0; i < SDL_arraysize(gui->pads); ++i)
        if (gui->pads[i]) SDL_GameControllerClose(gui->pads[i]);
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
    SDL_RenderCopy(gui->ren, gui->tex, NULL, NULL);
    SDL_RenderPresent(gui->ren);
    if (!gui->vsync) SDL_Delay(10);
}

/* Keeps gui->pads in step with hotplugging. */
static void gui_track_pads(Gui* gui, const SDL_Event* ev)
{
    if (ev->type == SDL_CONTROLLERDEVICEADDED) {
        int idx = ev->cdevice.which;
        for (size_t i = 0; i < SDL_arraysize(gui->pads); ++i) {
            if (!gui->pads[i]) {
                gui->pads[i] = SDL_GameControllerOpen(idx);
                break;
            }
        }
    } else if (ev->type == SDL_CONTROLLERDEVICEREMOVED) {
        for (size_t i = 0; i < SDL_arraysize(gui->pads); ++i) {
            SDL_Joystick* joy = gui->pads[i] ? SDL_GameControllerGetJoystick(gui->pads[i]) : NULL;
            if (joy && SDL_JoystickInstanceID(joy) == ev->cdevice.which) {
                SDL_GameControllerClose(gui->pads[i]);
                gui->pads[i] = NULL;
            }
        }
    }
}

/* Event → navigation. In the menu, gamepads navigate (D-pad, left stick,
   A / Start, B / Back); in a device view only holding Back leaves, so that
   every button can be tested. */
static GuiAction gui_action(Gui* gui, const SDL_Event* ev, bool in_menu, int* pick)
{
    switch (ev->type) {
    case SDL_QUIT:
        return GUI_QUIT;
    case SDL_KEYDOWN: {
        SDL_Keycode k = ev->key.keysym.sym;
        if (k == SDLK_c && (ev->key.keysym.mod & KMOD_CTRL)) return GUI_QUIT;
        if (k == SDLK_ESCAPE || k == SDLK_q || k == SDLK_AC_BACK) return GUI_BACK;
        if (!in_menu) return GUI_NONE;
        if (k == SDLK_UP || k == SDLK_k) return GUI_UP;
        if (k == SDLK_DOWN || k == SDLK_j) return GUI_DOWN;
        if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) return GUI_CONFIRM;
        if (k >= SDLK_0 && k <= SDLK_9) {
            *pick = (int)(k - SDLK_0);
            return GUI_PICK;
        }
        return GUI_NONE;
    }
    case SDL_MOUSEBUTTONDOWN:
        if (ev->button.button != SDL_BUTTON_LEFT) return GUI_NONE;
        if (in_menu) {
            *pick = graph_ui_menu_hit(&gui->ui, ev->button.x, ev->button.y);
            return *pick >= 0 ? GUI_PICK : GUI_NONE;
        }
        return graph_ui_back_hit(&gui->ui, ev->button.x, ev->button.y) ? GUI_BACK : GUI_NONE;
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP: {
        bool down = ev->type == SDL_CONTROLLERBUTTONDOWN;
        Uint8 b = ev->cbutton.button;
        if (!in_menu) {
            if (b == SDL_CONTROLLER_BUTTON_BACK) gui_hold_set(&gui->hold, down, SDL_GetTicks());
            return GUI_NONE;
        }
        if (!down) return GUI_NONE;
        if (b == SDL_CONTROLLER_BUTTON_DPAD_UP) return GUI_UP;
        if (b == SDL_CONTROLLER_BUTTON_DPAD_DOWN) return GUI_DOWN;
        if (b == SDL_CONTROLLER_BUTTON_A || b == SDL_CONTROLLER_BUTTON_START) return GUI_CONFIRM;
        if (b == SDL_CONTROLLER_BUTTON_B || b == SDL_CONTROLLER_BUTTON_BACK) return GUI_BACK;
        return GUI_NONE;
    }
    case SDL_CONTROLLERAXISMOTION:
        if (in_menu && ev->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
            return gui_stick_axis(&gui->stick, ev->caxis.value);
        return GUI_NONE;
    default:
        return GUI_NONE;
    }
}

static const char* kViewHint = "Esc / Back key / hold Select: menu";

/* Per-frame part of a device view: the hold-to-leave check. */
static bool gui_view_hold_done(Gui* gui)
{
    gui->ui.hold_progress = gui_hold_progress(&gui->hold, SDL_GetTicks(), GUI_HOLD_MS);
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

static GuiAction gui_test_joystick(Gui* gui, int joy_idx)
{
    SDL_Joystick* joy = SDL_JoystickOpen(joy_idx);
    if (!joy) {
        fprintf(stderr, "error: unable to open joystick %d: %s\n", joy_idx, SDL_GetError());
        return GUI_BACK;
    }

    int num_axes = SDL_JoystickNumAxes(joy);
    int num_buttons = SDL_JoystickNumButtons(joy);
    int num_hats = SDL_JoystickNumHats(joy);
    int num_balls = SDL_JoystickNumBalls(joy);
    if (num_axes < 0 || num_buttons < 0 || num_hats < 0 || num_balls < 0) {
        fprintf(stderr, "error: unable to query joystick layout: %s\n", SDL_GetError());
        SDL_JoystickClose(joy);
        return GUI_BACK;
    }
    Sint16* axes = calloc((size_t)num_axes + 1, sizeof(Sint16));
    Uint8* buttons = calloc((size_t)num_buttons + 1, sizeof(Uint8));
    Uint8* hats = calloc((size_t)num_hats + 1, sizeof(Uint8));
    Sint16* balls = calloc((size_t)num_balls + 1, 2 * sizeof(Sint16));
    SDL_JoystickID open_iid = SDL_JoystickInstanceID(joy);
    for (int i = 0; i < num_axes; ++i) axes[i] = SDL_JoystickGetAxis(joy, i);
    for (int i = 0; i < num_buttons; ++i) buttons[i] = SDL_JoystickGetButton(joy, i);
    for (int i = 0; i < num_hats; ++i) hats[i] = SDL_JoystickGetHat(joy, i);

    gui_view_begin(gui, SDL_JoystickName(joy));
    graph_ui_log(&gui->ui, "Opened joystick %d '%s' (instance %d)", joy_idx,
                 SDL_JoystickName(joy) ? SDL_JoystickName(joy) : "?", (int)open_iid);

    GuiAction result = GUI_NONE;
    while (result == GUI_NONE && !g_app_quit) {
        SDL_Event event;
        while (result == GUI_NONE && SDL_PollEvent(&event)) {
            int pick = -1;
            gui_track_pads(gui, &event);
            GuiAction act = gui_action(gui, &event, false, &pick);
            if (act == GUI_BACK || act == GUI_QUIT) {
                result = act;
                break;
            }
            switch (event.type) {
            case SDL_JOYAXISMOTION:
                if (event.jaxis.which != open_iid) break;
                if (event.jaxis.axis < num_axes) axes[event.jaxis.axis] = event.jaxis.value;
                graph_ui_log(&gui->ui, "%s axis=%d value=%+d",
                    sdl_event_name(event.type), event.jaxis.axis, event.jaxis.value);
                break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                if (event.jbutton.which != open_iid) break;
                if (event.jbutton.button < num_buttons) buttons[event.jbutton.button] = event.jbutton.state;
                graph_ui_log(&gui->ui, "%s button=%d %s",
                    sdl_event_name(event.type), event.jbutton.button,
                    event.jbutton.state ? "DOWN" : "UP");
                break;
            case SDL_JOYHATMOTION:
                if (event.jhat.which != open_iid) break;
                if (event.jhat.hat < num_hats) hats[event.jhat.hat] = event.jhat.value;
                graph_ui_log(&gui->ui, "%s hat=%d value=0x%02x",
                    sdl_event_name(event.type), event.jhat.hat, event.jhat.value);
                break;
            case SDL_JOYBALLMOTION:
                if (event.jball.which != open_iid) break;
                if (event.jball.ball < num_balls) {
                    balls[2*event.jball.ball] = event.jball.xrel;
                    balls[2*event.jball.ball+1] = event.jball.yrel;
                }
                graph_ui_log(&gui->ui, "%s ball=%d dx=%d dy=%d",
                    sdl_event_name(event.type), event.jball.ball,
                    event.jball.xrel, event.jball.yrel);
                break;
            case SDL_JOYDEVICEREMOVED:
                if (event.jdevice.which == open_iid) {
                    fprintf(stderr, "joystick %d: disconnected\n", joy_idx);
                    result = GUI_BACK;
                }
                break;
            default:
                break;
            }
        }
        if (result == GUI_NONE && gui_view_hold_done(gui)) result = GUI_BACK;
        graph_ui_draw_joystick(&gui->ui, SDL_JoystickName(joy), joy_idx,
            axes, num_axes, buttons, num_buttons, NULL, NULL, 0,
            hats, num_hats, balls, num_balls);
        gui_present(gui);
    }
    free(balls); free(hats); free(buttons); free(axes);
    SDL_JoystickClose(joy);
    return g_app_quit ? GUI_QUIT : result;
}

static GuiAction gui_test_gamecontroller(Gui* gui, int idx)
{
    SDL_GameController* pad = SDL_GameControllerOpen(idx);
    if (!pad) return gui_test_joystick(gui, idx);

    SDL_Joystick* joy = SDL_GameControllerGetJoystick(pad);
    int num_axes = SDL_CONTROLLER_AXIS_MAX;
    int num_buttons = SDL_CONTROLLER_BUTTON_MAX;
    Sint16 axes[SDL_CONTROLLER_AXIS_MAX];
    Uint8 buttons[SDL_CONTROLLER_BUTTON_MAX];
    const char* labels[SDL_CONTROLLER_BUTTON_MAX];
    for (int i = 0; i < num_buttons; ++i) {
        const char* s = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)i);
        labels[i] = s ? s : "";
        buttons[i] = SDL_GameControllerGetButton(pad, (SDL_GameControllerButton)i) ? 1 : 0;
    }
    for (int i = 0; i < num_axes; ++i)
        axes[i] = SDL_GameControllerGetAxis(pad, (SDL_GameControllerAxis)i);
    SDL_JoystickID open_iid = joy ? SDL_JoystickInstanceID(joy) : -1;

    gui_view_begin(gui, SDL_GameControllerName(pad));
    graph_ui_log(&gui->ui, "GameController '%s' (instance %d)",
                 SDL_GameControllerName(pad) ? SDL_GameControllerName(pad) : "?", (int)open_iid);

    GuiAction result = GUI_NONE;
    while (result == GUI_NONE && !g_app_quit) {
        SDL_Event event;
        while (result == GUI_NONE && SDL_PollEvent(&event)) {
            int pick = -1;
            gui_track_pads(gui, &event);
            GuiAction act = gui_action(gui, &event, false, &pick);
            if (act == GUI_BACK || act == GUI_QUIT) {
                result = act;
                break;
            }
            switch (event.type) {
            case SDL_CONTROLLERAXISMOTION:
                if (event.caxis.which != open_iid) break;
                if (event.caxis.axis < num_axes) axes[event.caxis.axis] = event.caxis.value;
                graph_ui_log(&gui->ui, "%s axis=%d (%s) value=%+d", sdl_event_name(event.type),
                    event.caxis.axis,
                    SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)event.caxis.axis),
                    event.caxis.value);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                if (event.cbutton.which != open_iid) break;
                if (event.cbutton.button < num_buttons) buttons[event.cbutton.button] = event.cbutton.state;
                graph_ui_log(&gui->ui, "%s button=%d (%s) %s", sdl_event_name(event.type),
                    event.cbutton.button, labels[event.cbutton.button < num_buttons ? event.cbutton.button : 0],
                    event.cbutton.state ? "DOWN" : "UP");
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                if (event.cdevice.which == open_iid) {
                    fprintf(stderr, "gamecontroller %d: disconnected\n", idx);
                    result = GUI_BACK;
                }
                break;
            default:
                break;
            }
        }
        if (result == GUI_NONE && gui_view_hold_done(gui)) result = GUI_BACK;
        TestUIButtonGroup pad_groups[] = {{0,4},{4,7},{7,11},{11,15},{15,num_buttons}};
        int ng = num_buttons <= 15 ? 4 : 5;
        graph_ui_draw_joystick(&gui->ui, SDL_GameControllerName(pad),
            SDL_GameControllerGetPlayerIndex(pad),
            axes, num_axes, buttons, num_buttons, labels, pad_groups, ng,
            NULL, 0, NULL, 0);
        gui_present(gui);
    }
    SDL_GameControllerClose(pad);
    return g_app_quit ? GUI_QUIT : result;
}

/* Device menu, rebuilt every frame so that hotplugging shows at once; the
   last row quits (touch has no other way). Returns the joystick index, or
   -1 to quit. */
static int gui_menu(Gui* gui)
{
    SDL_SetWindowTitle(gui->win, DISPLAY_NAME " - select device");
    gui->ui.hint = "Up/Down  Enter/A: open  Esc/B: quit";
    gui->ui.hold_progress = 0.0f;
    memset(&gui->stick, 0, sizeof(gui->stick));
    for (;;) {
        if (g_app_quit) return -1;
        int njoy = SDL_NumJoysticks();
        if (njoy < 0) njoy = 0;
        int nitems = njoy + 1;
        char** labels = calloc((size_t)nitems, sizeof(char*));
        for (int i = 0; i < njoy; ++i) {
            char buf[256];
            const char* name = SDL_JoystickNameForIndex(i);
            snprintf(buf, sizeof(buf), "%s  %s", SDL_IsGameController(i) ? "[pad]" : "[joy]",
                     name ? name : "(unknown)");
            labels[i] = SDL_strdup(buf);
        }
        labels[njoy] = SDL_strdup(njoy ? "Quit" : "Quit   (no devices yet: connect one, it shows up here)");
        gui->ui.menu_numbered = njoy;

        int choice = -2;  /* -2: stay */
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
            graph_ui_draw_menu(&gui->ui, "Select joystick / gamecontroller",
                               (const char* const*)labels, nitems);
            gui_present(gui);
        }
        for (int i = 0; i < nitems; ++i) SDL_free(labels[i]);
        free(labels);
        if (choice != -2) return choice;
    }
}

static GuiAction gui_test_device(Gui* gui, int idx)
{
    return SDL_IsGameController(idx) ? gui_test_gamecontroller(gui, idx) : gui_test_joystick(gui, idx);
}

/* --gui [JOYNUM], and the default without a terminal UI. With JOYNUM,
   leaving the device view ends the program. */
static int gui_run(const JstestOptions* opts)
{
    Gui gui;
    if (!gui_open(&gui, opts)) return EXIT_FAILURE;
    if (opts->joy_idx >= 0) {
        gui_test_device(&gui, opts->joy_idx);
    } else {
        for (;;) {
            int sel = gui_menu(&gui);
            if (sel < 0) break;
            if (gui_test_device(&gui, sel) == GUI_QUIT) break;
            gui.ui.hint = NULL;
        }
    }
    gui_close(&gui);
    return EXIT_SUCCESS;
}

/* ---- Startup ---- */

/* gamecontrollerdb.txt, read through SDL (Android: APK asset) and kept for
   the report's mapping sources. */
typedef struct MappingDb {
    char path[4200];
    char* text;
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
    SDL_RWops* rw = SDL_RWFromFile(db->path, "rb");
    Sint64 size = rw ? SDL_RWsize(rw) : -1;
    if (rw && size >= 0) {
        db->text = malloc((size_t)size + 1);
        if (db->text && SDL_RWread(rw, db->text, 1, (size_t)size) == (size_t)size) {
            db->text[size] = '\0';
            db->count = SDL_GameControllerAddMappingsFromRW(SDL_RWFromConstMem(db->text, (int)size), 1);
        }
    }
    if (db->count < 0)
        snprintf(db->error, sizeof(db->error), "%s: %s", db->path, SDL_GetError());
    if (rw) SDL_RWclose(rw);
}

static void report_startup(const DataDir* data_dir, const MappingDb* db)
{
    char lib[512];
    SDL_version linked, compiled;
    SDL_GetVersion(&linked);
    SDL_VERSION(&compiled);
    const char* mapped = report_mapped_library("libSDL2", lib, sizeof(lib));
    report_line("sdl", "%d.%d.%d (built against %d.%d.%d), %s%s%s", linked.major, linked.minor,
                linked.patch, compiled.major, compiled.minor, compiled.patch, SDL_GetRevision(),
                mapped ? ", " : "", mapped ? mapped : "");
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
        SDL_RendererInfo info;
        if (SDL_GetRenderDriverInfo(i, &info) != 0) continue;
        size_t len = strlen(renderers);
        snprintf(renderers + len, sizeof(renderers) - len, "%s%s", len ? " " : "", info.name);
    }
    report_line("render drivers", "%s", renderers);

    int njoy = SDL_NumJoysticks();
    report_line("joysticks", "%d, %d haptic devices, %d touch devices", njoy, SDL_NumHaptics(),
                SDL_GetNumTouchDevices());
    for (int i = 0; i < njoy; ++i) {
        char guid[64];
        SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i), guid, sizeof(guid));
        char topic[32];
        snprintf(topic, sizeof(topic), "joystick %d", i);
        const char* name = SDL_JoystickNameForIndex(i);
        report_line(topic, "%s, GUID %s", name ? name : "?", guid);
        if (SDL_IsGameController(i)) {
            char* mapping = SDL_GameControllerMappingForDeviceIndex(i);
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
}

/* Video is needed for the event queue, but not for the terminal modes: on a
   console without display those fall back to the dummy driver (LOG-6). */
static bool init_sdl(JstestMode mode)
{
    /* Sensors, touchpads, SDL_POLLSENTINEL, ... are called unconditionally;
       a binary built against newer headers (R36S) must not run them on an
       older library (ArkOS' default is 2.0.10). */
    SDL_version linked;
    SDL_GetVersion(&linked);
    if (SDL_VERSIONNUM(linked.major, linked.minor, linked.patch) < SDL_VERSIONNUM(2, 0, 18)) {
        fprintf(stderr, "error: SDL %d.%d.%d is too old, sdl2-jstest needs 2.0.18 or newer\n",
                linked.major, linked.minor, linked.patch);
        return false;
    }

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) < 0) {
        fprintf(stderr, "error: unable to init SDL: %s\n", SDL_GetError());
        return false;
    }
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        if (mode == JSTEST_MODE_GUI) {
            fprintf(stderr, "error: unable to init SDL video: %s\n", SDL_GetError());
            return false;
        }
        fprintf(stderr, "video: unavailable (%s), using the dummy driver\n", SDL_GetError());
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
            fprintf(stderr, "error: unable to init SDL video: %s\n", SDL_GetError());
            return false;
        }
    }
    return true;
}

int main(int argc, char** argv)
{
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
        printf("sdl2-jstest %s\n", SDL_JSTEST_VERSION);
        return EXIT_SUCCESS;
    }
    platform_find_data_dir(&data_dir, opts.data_dir, platform_exe_dir(), SDL2_JSTEST_DATADIR);
    if (opts.mode == JSTEST_MODE_SHOW_PATHS) {
        platform_print_paths(&data_dir);
        return EXIT_SUCCESS;
    }

    /* Android starts main() again in the same process. */
    g_app_quit = 0;

    report_program("sdl2-jstest", SDL_JSTEST_VERSION);
    report_system(SDL_GetPlatform(), SDL_GetCPUCount(), SDL_GetSystemRAM());
    report_env();

    // Without a window, SDL2 only reports joystick events with this hint.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    // Android: the Back key arrives as SDLK_AC_BACK instead of closing.
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    // Window matching with the launcher (INT-3); names, since older SDL2
    // headers (R36S: 2.0.10) lack some of the macros.
    SDL_SetHint("SDL_VIDEO_X11_WMCLASS", "sdl2-jstest");
    SDL_SetHint("SDL_VIDEO_WAYLAND_WMCLASS", "sdl2-jstest");
    SDL_SetHint("SDL_APP_NAME", DISPLAY_NAME);
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

    free(db.text);
    SDL_Quit();
    return status;
}

/* EOF */
