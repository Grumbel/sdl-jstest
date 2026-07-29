// SPDX-FileCopyrightText: 2015-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <SDL3/SDL.h>
#include <assert.h>
#include <curses.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define DUALSENSE_GAMEPAD_TYPE SDL_Gamepad
#define DUALSENSE_SEND_EFFECT  SDL_SendGamepadEffect
#include "utils/dualsense.h"


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


static void print_bar(int pos, int len)
{
    addch('[');
    for (int i = 0; i < len; ++i)
    {
        if (i == pos)
            addch('#');
        else
            addch(' ');
    }
    addch(']');
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
    }
    else
    {
        initscr();

        //cbreak();
        noecho();
        nodelay(stdscr, true);
        //nonl();
        curs_set(0);

        int num_axes = SDL_GetNumJoystickAxes(joy);
        if (num_axes < 0)
        {
            fprintf(stderr, "Unable to get SDL axes count: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }

        int num_buttons = SDL_GetNumJoystickButtons(joy);
        if (num_buttons < 0)
        {
            fprintf(stderr, "Unable to get SDL buttons count: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }

        int num_hats = SDL_GetNumJoystickHats(joy);
        if (num_hats < 0)
        {
            fprintf(stderr, "Unable to get SDL hats count: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }

        int num_balls = SDL_GetNumJoystickBalls(joy);
        if (num_balls < 0)
        {
            fprintf(stderr, "Unable to get SDL balls count: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }

        Sint16* axes = calloc((size_t)num_axes, sizeof(Sint16));
        Uint8* buttons = calloc((size_t)num_buttons, sizeof(Uint8));
        Uint8* hats = calloc((size_t)num_hats, sizeof(Uint8));
        Sint16* balls = calloc((size_t)num_balls, 2 * sizeof(Sint16));

        int quit = 0;
        SDL_Event event;
        while (!quit)
        {
            SDL_Delay(10);

            bool something_new = false;
            while (SDL_PollEvent(&event))
            {
                something_new = true;
                switch (event.type)
                {
                case SDL_EVENT_JOYSTICK_AXIS_MOTION:
                    assert(event.jaxis.axis < num_axes);
                    axes[event.jaxis.axis] = event.jaxis.value;
                    break;

                case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
                case SDL_EVENT_JOYSTICK_BUTTON_UP:
                    assert(event.jbutton.button < num_buttons);
                    buttons[event.jbutton.button] = event.jbutton.down;
                    break;

                case SDL_EVENT_JOYSTICK_HAT_MOTION:
                    assert(event.jhat.hat < num_hats);
                    hats[event.jhat.hat] = event.jhat.value;
                    break;

                case SDL_EVENT_JOYSTICK_BALL_MOTION:
                    assert(event.jball.ball < num_balls);
                    balls[2 * event.jball.ball + 0] = event.jball.xrel;
                    balls[2 * event.jball.ball + 1] = event.jball.yrel;
                    break;

                case SDL_EVENT_CLIPBOARD_UPDATE:
                    break;

                case SDL_EVENT_KEYMAP_CHANGED:
                    break;

                case SDL_EVENT_QUIT:
                    quit = 1;
                    printf("Recieved interrupt, exiting\n");
                    break;

                default:
                    fprintf(stderr, "Error: Unhandled event type: %d\n", event.type);
                }
            }

            if (something_new)
            {
                //clear();
                move(0, 0);

                printw("Joystick Name:   '%s'\n", SDL_GetJoystickName(joy));
                printw("Joystick Number: %d\n", joy_idx);
                printw("\n");

                printw("Axes %2d:\n", num_axes);
                for (int i = 0; i < num_axes; ++i)
                {
                    int len = COLS - 20;
                    printw("  %2d: %6d  ", i, axes[i]);
                    print_bar((axes[i] + 32767) * (len - 1) / 65534, len);
                    addch('\n');
                }
                printw("\n");

                printw("Buttons %2d:\n", num_buttons);
                for (int i = 0; i < num_buttons; ++i)
                {
                    printw("  %2d: %d  %s\n", i, buttons[i], buttons[i] ? "[#]" : "[ ]");
                }
                printw("\n");

                printw("Hats %2d:\n", num_hats);
                for (int i = 0; i < num_hats; ++i)
                {
                    printw("  %2d: value: %d\n", i, hats[i]);
                    printw("  +-----+  up:    %c\n"
                           "  |%c %c %c|  down:  %c\n"
                           "  |%c %c %c|  left:  %c\n"
                           "  |%c %c %c|  right: %c\n"
                           "  +-----+\n",

                           (hats[i] & SDL_HAT_UP) ? '1' : '0',

                           ((hats[i] & SDL_HAT_UP) && (hats[i] & SDL_HAT_LEFT)) ? 'O' : ' ',
                           ((hats[i] & SDL_HAT_UP) && !(hats[i] & (SDL_HAT_LEFT | SDL_HAT_RIGHT))) ? 'O' : ' ',
                           ((hats[i] & SDL_HAT_UP) && (hats[i] & SDL_HAT_RIGHT)) ? 'O' : ' ',

                           (hats[i] & SDL_HAT_DOWN) ? '1' : '0',

                           (!(hats[i] & (SDL_HAT_UP | SDL_HAT_DOWN)) && (hats[i] & SDL_HAT_LEFT)) ? 'O' : ' ',
                           (!(hats[i] & (SDL_HAT_UP | SDL_HAT_DOWN)) && !(hats[i] & (SDL_HAT_LEFT | SDL_HAT_RIGHT)))
                               ? 'O'
                               : ' ',
                           (!(hats[i] & (SDL_HAT_UP | SDL_HAT_DOWN)) && (hats[i] & SDL_HAT_RIGHT)) ? 'O' : ' ',

                           (hats[i] & SDL_HAT_LEFT) ? '1' : '0',

                           ((hats[i] & SDL_HAT_DOWN) && (hats[i] & SDL_HAT_LEFT)) ? 'O' : ' ',
                           ((hats[i] & SDL_HAT_DOWN) && !(hats[i] & (SDL_HAT_LEFT | SDL_HAT_RIGHT))) ? 'O' : ' ',
                           ((hats[i] & SDL_HAT_DOWN) && (hats[i] & SDL_HAT_RIGHT)) ? 'O' : ' ',

                           (hats[i] & SDL_HAT_RIGHT) ? '1' : '0');
                }
                printw("\n");

                printw("Balls %2d: ", num_balls);
                for (int i = 0; i < num_balls; ++i)
                {
                    printw("  %2d: %6d %6d\n", i, balls[2 * i + 0], balls[2 * i + 1]);
                }
                printw("\n");
                printw("\n");
                printw("Press Ctrl-c to exit\n");

                refresh();
            }

            if (getch() == 3) // Ctrl-c
            {
                quit = 1;
            }
        } // while

        free(balls);
        free(hats);
        free(buttons);
        free(axes);

        endwin();
    }
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

    printf("Entering gamecontroller test loop, press Ctrl-c to exit\n");
    int quit = 0;
    SDL_Event event;

    initscr();

    // cbreak();
    noecho();
    nodelay(stdscr, true);
    // nonl();
    curs_set(0);

    int num_axes = SDL_GAMEPAD_AXIS_COUNT;
    int num_buttons = SDL_GAMEPAD_BUTTON_COUNT;

    Sint16* axes = calloc((size_t)num_axes, sizeof(Sint16));
    Uint8* buttons = calloc((size_t)num_buttons, sizeof(Uint8));

    bool has_gyro = SDL_GamepadHasSensor(gamepad, SDL_SENSOR_GYRO);
    if (has_gyro)
    {
        SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_GYRO, true);
    }
    bool has_accel = SDL_GamepadHasSensor(gamepad, SDL_SENSOR_ACCEL);
    if (has_accel)
    {
        SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_ACCEL, true);
    }
    float gyro[3] = {0.0f, 0.0f, 0.0f};
    float accel[3] = {0.0f, 0.0f, 0.0f};

    SDL_PropertiesID props = SDL_GetGamepadProperties(gamepad);
    bool has_led = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_RGB_LED_BOOLEAN, false)
        || SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_MONO_LED_BOOLEAN, false);
    int touchpads = SDL_GetNumGamepadTouchpads(gamepad);
    bool has_rumble = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);

    while (!quit && SDL_WaitEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
            {
                switch (event.gsensor.sensor)
                {
                case SDL_SENSOR_GYRO:
                    gyro[0] = event.gsensor.data[0];
                    gyro[1] = event.gsensor.data[1];
                    gyro[2] = event.gsensor.data[2];
                    break;
                case SDL_SENSOR_ACCEL:
                    accel[0] = event.gsensor.data[0];
                    accel[1] = event.gsensor.data[1];
                    accel[2] = event.gsensor.data[2];
                    break;
                }
                break;
            }
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            assert(event.gaxis.axis < num_axes);
            axes[event.gaxis.axis] = event.gaxis.value;
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            assert(event.gbutton.button < num_buttons);
            buttons[event.gbutton.button] = event.gbutton.down;

        /* Cycle PS5 trigger effects when the microphone button is pressed */
            if (event.gbutton.button == SDL_GAMEPAD_BUTTON_MISC1 &&
                SDL_GetGamepadType(gamepad) == SDL_GAMEPAD_TYPE_PS5)
            {
                CyclePS5TriggerEffect(gamepad);
            }
            break;
        case SDL_EVENT_QUIT:
            quit = 1;
            printf("Received interrupt, exiting\n");
            break;
        }

        if (trigger_effect == 0)
        {
            {
                /* Update rumble based on trigger state */
                Sint16 left = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
                Sint16 right = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
                Uint16 low_frequency_rumble = ConvertAxisToRumble(left);
                Uint16 high_frequency_rumble = ConvertAxisToRumble(right);
                SDL_RumbleGamepad(gamepad, low_frequency_rumble, high_frequency_rumble, 250);
            }
            {
                /* Update trigger rumble based on thumbstick state */
                Sint16 left = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
                Sint16 right = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY);
                Uint16 left_rumble = ConvertAxisToRumble(left);
                Uint16 right_rumble = ConvertAxisToRumble(right);

                SDL_RumbleGamepadTriggers(gamepad, left_rumble, right_rumble, 250);
            }
        }

        // clear();
        move(0, 0);

        printw("Joystick Name:   '%s'\n", SDL_GetGamepadName(gamepad));
        printw("Joystick Number: %d\n", SDL_GetGamepadPlayerIndex(gamepad));
        printw("\n");

        if (has_gyro)
            printw("Gyro:  [ %6.3f  %6.3f  %6.3f ]\n", gyro[0], gyro[1], gyro[2]);
        if (has_accel)
            printw("Accel: [ %6.3f  %6.3f  %6.3f ]\n", accel[0], accel[1], accel[2]);
        printw("\n");

        printw("Axes:\n");
        for (int i = 0; i < num_axes; ++i)
        {
            int len = COLS - 20;
            printw("  %2d: %6d  ", i, axes[i]);
            print_bar((axes[i] + 32767) * (len - 1) / 65534, len);
            addch('\n');
        }
        printw("\n");

        printw("Buttons:\n");
        // A, B, X, Y
        for (int i = 0; i < 4; ++i)
        {
            printw("  %9.9s: %s", SDL_GetGamepadStringForButton(i), buttons[i] ? "[#]" : "[ ]");
        }
        printw("\n");
        // Back, Guide, Start
        for (int i = 4; i < 7; ++i)
        {
            printw("  %9.9s: %s", SDL_GetGamepadStringForButton(i), buttons[i] ? "[#]" : "[ ]");
        }
        printw("\n");
        // Left stick, right stick, left shoulder, right shoulder
        for (int i = 7; i < 11; ++i)
        {
            printw("  %9.9s: %s", SDL_GetGamepadStringForButton(i), buttons[i] ? "[#]" : "[ ]");
        }
        printw("\n");
        // D-pad
        for (int i = 11; i < 15; ++i)
        {
            printw("  %9.9s: %s", SDL_GetGamepadStringForButton(i), buttons[i] ? "[#]" : "[ ]");
        }
        printw("\n");
        // Rest
        for (int i = 15; i < num_buttons; ++i)
        {
            printw("  %9.9s: %s", SDL_GetGamepadStringForButton(i), buttons[i] ? "[#]" : "[ ]");
        }
        printw("\n");
        printw("\n");

        printw("Misc:\n");
        printw("  %9.9s: %s", "LED", has_led ? "[#]" : "[ ]");
        printw("  %9.9s: %d", "Touchpads", touchpads);
        printw("  %9.9s: %s", "Rumble", has_rumble ? "[#]" : "[ ]");

        printw("\n");
        printw("\n");
        printw("Press Ctrl-c to exit\n");

        refresh();
    }

    free(buttons);
    free(axes);
    endwin();
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
                fprintf(stderr, "Error: Unhandled event type: %d\n", event.type);
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

        if (argc == 2 && (strcmp(argv[1], "--help") == 0 ||
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
        else
        {
            fprintf(stderr, "%s: unknown arguments\n", argv[0]);
            fprintf(stderr, "Try '%s --help' for more informations\n", argv[0]);
        }
    }

    return EXIT_SUCCESS;
}

/* EOF */
