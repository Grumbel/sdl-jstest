// SPDX-FileCopyrightText: 2014-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-FileCopyrightText: 2017 Jaime Moreira <jmoreirag@gmail.com>
// SPDX-FileCopyrightText: 2018 Anthony J. Bentley <anthony@anjbe.name>
// SPDX-FileCopyrightText: 2022 orbea <orbea@riseup.net>
// SPDX-FileCopyrightText: 2024 GhostlyDark <31742919+GhostlyDark@users.noreply.github.com>
// SPDX-FileCopyrightText: 2024-2025 ABeltramo <beltramo.ale@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <SDL.h>
#include <assert.h>
#include <curses.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils/dualsense.h"
#include "ui/test_ui.h"


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

static void print_help(const char* prg)
{
    printf("Usage: %s [OPTION]\n", prg);
    printf("List available joysticks or test a joystick.\n");
    printf("This programm uses SDL for doing its test instead of using the raw\n"
        "/dev/input/jsX interface\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help             Print this help\n");
    printf("  --version              Print version number and exit\n");
    printf("  -l, --list             Search for available joysticks and list their properties\n");
    printf("  -t, --test JOYNUM      Display a graphical representation of the current joystick state\n");
    printf("  -g, --gamecontroller IDX\n"
        "                         Test GameController\n");
    printf("  -e, --event JOYNUM     Display the events that are received from the joystick\n");
    printf("  -r, --rumble JOYNUM    Test rumble effects on gamepad JOYNUM\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s --list\n", prg);
    printf("  %s --test 1\n", prg);
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

    TestUI ui;
    test_ui_init(&ui);
    test_ui_log(&ui, "Opened joystick %d '%s'", joy_idx,
                SDL_JoystickName(joy) ? SDL_JoystickName(joy) : "?");

    int quit = 0;
    SDL_Event event;
    while (!quit)
    {
        SDL_Delay(10);
        bool redraw = false;
        while (SDL_PollEvent(&event))
        {
            redraw = true;
            switch (event.type)
            {
            case SDL_JOYAXISMOTION:
                if (event.jaxis.axis < num_axes)
                    axes[event.jaxis.axis] = event.jaxis.value;
                test_ui_log(&ui, "AXIS  joy=%d axis=%d value=%+d",
                            event.jaxis.which, event.jaxis.axis, event.jaxis.value);
                break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                if (event.jbutton.button < num_buttons)
                    buttons[event.jbutton.button] = event.jbutton.state;
                test_ui_log(&ui, "BUTTON joy=%d btn=%d %s",
                            event.jbutton.which, event.jbutton.button,
                            event.jbutton.state ? "DOWN" : "UP");
                break;
            case SDL_JOYHATMOTION:
                if (event.jhat.hat < num_hats)
                    hats[event.jhat.hat] = event.jhat.value;
                test_ui_log(&ui, "HAT    joy=%d hat=%d value=%d",
                            event.jhat.which, event.jhat.hat, event.jhat.value);
                break;
            case SDL_JOYBALLMOTION:
                if (event.jball.ball < num_balls)
                {
                    balls[2 * event.jball.ball] = event.jball.xrel;
                    balls[2 * event.jball.ball + 1] = event.jball.yrel;
                }
                test_ui_log(&ui, "BALL   joy=%d ball=%d dx=%d dy=%d",
                            event.jball.which, event.jball.ball,
                            event.jball.xrel, event.jball.yrel);
                break;
            case SDL_JOYDEVICEADDED:
                test_ui_log(&ui, "DEVICE ADDED   which=%d", event.jdevice.which);
                break;
            case SDL_JOYDEVICEREMOVED:
                test_ui_log(&ui, "DEVICE REMOVED which=%d", event.jdevice.which);
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
        if (redraw || ui.dirty)
        {
            test_ui_draw_joystick(&ui, SDL_JoystickName(joy), joy_idx,
                                  axes, num_axes, buttons, num_buttons, NULL,
                                  NULL, 0,
                                  hats, num_hats, balls, num_balls);
            test_ui_refresh(&ui);
        }
        if (test_ui_check_quit(&ui))
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

    TestUI ui;
    test_ui_init(&ui);
    test_ui_log(&ui, "GameController '%s'", SDL_GameControllerName(gamepad));

    int quit = 0;
    SDL_Event event;
    while (!quit)
    {
        SDL_Delay(10);
        bool redraw = false;
        while (SDL_PollEvent(&event))
        {
            redraw = true;
            switch (event.type)
            {
            case SDL_CONTROLLERSENSORUPDATE:
                test_ui_log(&ui, "SENSOR type=%d", event.csensor.sensor);
                break;
            case SDL_CONTROLLERAXISMOTION:
                if (event.caxis.axis < num_axes)
                    axes[event.caxis.axis] = event.caxis.value;
                break;
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                if (event.cbutton.button < num_buttons)
                    buttons[event.cbutton.button] = event.cbutton.state;
                test_ui_log(&ui, "BUTTON %s %s",
                            SDL_GameControllerGetStringForButton(event.cbutton.button),
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
                test_ui_log(&ui, "%s which=%d", sdl_event_name(event.type), event.cdevice.which);
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                test_ui_log(&ui, "CONTROLLER REMOVED which=%d", event.cdevice.which);
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
        if (redraw || ui.dirty)
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
        if (test_ui_check_quit(&ui))
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

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        print_help(argv[0]);
        exit(1);
    }

    // SDL2 will only report events when the window has focus, so set
    // this hint as we don't have a window
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    // FIXME: We don't need video, but without it SDL will fail to work in SDL_WaitEvent()
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) < 0)
    {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
    }
    else
    {
        atexit(SDL_Quit);

        {
            int ret = SDL_GameControllerAddMappingsFromFile(SDL2_JSTEST_DATADIR "/gamecontrollerdb.txt");
            if (ret < 0)
            {
                ret = SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
            }

            if (ret < 0)
            {
                fprintf(stderr, "error: failed to read gamecontrollerdb.txt: %s\n", SDL_GetError());
            }
        }

        if (argc == 2 && (strcmp(argv[1], "--help") == 0 ||
            strcmp(argv[1], "-h") == 0))
        {
            print_help(argv[0]);
        }
        else if (argc == 2 && (strcmp(argv[1], "--version") == 0))
        {
            printf("sdl2-jstest %s\n", SDL_JSTEST_VERSION);
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
        else
        {
            fprintf(stderr, "%s: unknown arguments\n", argv[0]);
            fprintf(stderr, "Try '%s --help' for more informations\n", argv[0]);
        }
    }

    return EXIT_SUCCESS;
}

/* EOF */
