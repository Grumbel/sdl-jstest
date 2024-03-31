// sdl-jstest - Joystick Test Program for SDL
// Copyright (C) 2014 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <SDL.h>
#include <assert.h>
#include <curses.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static void print_bar(int pos, int len)
{
  addch('[');
  for(int i = 0; i < len; ++i)
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
  if (errno != 0) {
    return 0;
  }

  // garbage at the end
  if (*endptr != '\0') {
    return 0;
  }

  // out of range of int
  if (tmp < INT_MIN || tmp > INT_MAX) {
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
    for(int joy_idx = 0; joy_idx < num_joysticks; ++joy_idx)
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
  }
  else
  {
    initscr();

    //cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    //nonl();
    curs_set(0);

    int num_axes    = SDL_JoystickNumAxes(joy);
    if (num_axes < 0) {
      fprintf(stderr, "Unable to get SDL axes count: %s\n", SDL_GetError());
      exit(EXIT_FAILURE);
    }

    int num_buttons = SDL_JoystickNumButtons(joy);
    if (num_buttons < 0) {
      fprintf(stderr, "Unable to get SDL buttons count: %s\n", SDL_GetError());
      exit(EXIT_FAILURE);
    }

    int num_hats    = SDL_JoystickNumHats(joy);
    if (num_hats < 0) {
      fprintf(stderr, "Unable to get SDL hats count: %s\n", SDL_GetError());
      exit(EXIT_FAILURE);
    }

    int num_balls   = SDL_JoystickNumBalls(joy);
    if (num_balls < 0) {
      fprintf(stderr, "Unable to get SDL balls count: %s\n", SDL_GetError());
      exit(EXIT_FAILURE);
    }

    Sint16* axes    = calloc((size_t)num_axes,    sizeof(Sint16));
    Uint8*  buttons = calloc((size_t)num_buttons, sizeof(Uint8));
    Uint8*  hats    = calloc((size_t)num_hats,    sizeof(Uint8));
    Sint16*  balls   = calloc((size_t)num_balls,   2*sizeof(Sint16));

    int quit = 0;
    SDL_Event event;
    while(!quit)
    {
      SDL_Delay(10);

      bool something_new = FALSE;
      while (SDL_PollEvent(&event)) {
        something_new = TRUE;
        switch(event.type)
        {
          case SDL_JOYAXISMOTION:
            assert(event.jaxis.axis < num_axes);
            axes[event.jaxis.axis] = event.jaxis.value;
            break;

          case SDL_JOYBUTTONDOWN:
          case SDL_JOYBUTTONUP:
            assert(event.jbutton.button < num_buttons);
            buttons[event.jbutton.button] = event.jbutton.state;
            break;

          case SDL_JOYHATMOTION:
            assert(event.jhat.hat < num_hats);
            hats[event.jhat.hat] = event.jhat.value;
            break;

          case SDL_JOYBALLMOTION:
            assert(event.jball.ball < num_balls);
            balls[2*event.jball.ball + 0] = event.jball.xrel;
            balls[2*event.jball.ball + 1] = event.jball.yrel;
            break;

          case SDL_QUIT:
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
        move(0,0);

        printw("Joystick Name:   '%s'\n", SDL_JoystickName(joy));
        printw("Joystick Number: %d\n", joy_idx);
        printw("\n");

        printw("Axes %2d:\n", num_axes);
        for(int i = 0; i < num_axes; ++i)
        {
          int len = COLS - 20;
          printw("  %2d: %6d  ", i, axes[i]);
          print_bar((axes[i] + 32767) * (len-1) / 65534, len);
          addch('\n');
        }
        printw("\n");

        printw("Buttons %2d:\n", num_buttons);
        for(int i = 0; i < num_buttons; ++i)
        {
          printw("  %2d: %d  %s\n", i, buttons[i], buttons[i] ? "[#]":"[ ]");
        }
        printw("\n");

        printw("Hats %2d:\n", num_hats);
        for(int i = 0; i < num_hats; ++i)
        {
          printw("  %2d: value: %d\n", i, hats[i]);
          printw("  +-----+  up:    %c\n"
                 "  |%c %c %c|  down:  %c\n"
                 "  |%c %c %c|  left:  %c\n"
                 "  |%c %c %c|  right: %c\n"
                 "  +-----+\n",

                 (hats[i] & SDL_HAT_UP)?'1':'0',

                 ((hats[i] & SDL_HAT_UP) && (hats[i] & SDL_HAT_LEFT)) ? 'O' : ' ',
                 ((hats[i] & SDL_HAT_UP) && !(hats[i] & (SDL_HAT_LEFT | SDL_HAT_RIGHT))) ? 'O' : ' ',
                 ((hats[i] & SDL_HAT_UP) && (hats[i] & SDL_HAT_RIGHT)) ? 'O' : ' ',

                 (hats[i] & SDL_HAT_DOWN)?'1':'0',

                 (!(hats[i] & (SDL_HAT_UP | SDL_HAT_DOWN)) && (hats[i] & SDL_HAT_LEFT)) ? 'O' : ' ',
                 (!(hats[i] & (SDL_HAT_UP | SDL_HAT_DOWN)) && !(hats[i] & (SDL_HAT_LEFT | SDL_HAT_RIGHT))) ? 'O' : ' ',
                 (!(hats[i] & (SDL_HAT_UP | SDL_HAT_DOWN)) && (hats[i] & SDL_HAT_RIGHT)) ? 'O' : ' ',

                 (hats[i] & SDL_HAT_LEFT)?'1':'0',

                 ((hats[i] & SDL_HAT_DOWN) && (hats[i] & SDL_HAT_LEFT)) ? 'O' : ' ',
                 ((hats[i] & SDL_HAT_DOWN) && !(hats[i] & (SDL_HAT_LEFT | SDL_HAT_RIGHT))) ? 'O' : ' ',
                 ((hats[i] & SDL_HAT_DOWN) && (hats[i] & SDL_HAT_RIGHT)) ? 'O' : ' ',

                 (hats[i] & SDL_HAT_RIGHT)?'1':'0');
        }
        printw("\n");

        printw("Balls %2d: ", num_balls);
        for(int i = 0; i < num_balls; ++i)
        {
          printw("  %2d: %6d %6d\n", i, balls[2*i+0], balls[2*i+1]);
        }
        printw("\n");
        printw("\n");
        printw("Press Ctrl-c to exit\n");

        refresh();
      }

      if ( getch() == 3 ) // Ctrl-c
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
#if 0
static void test_gamecontroller_events(SDL_GameController* gamepad)
{
  assert(gamepad);

  printf("Entering gamecontroller test loop, press Ctrl-c to exit\n");
  int quit = 0;
  SDL_Event event;

  while(!quit && SDL_WaitEvent(&event))
  {
    switch(event.type)
    {
      case SDL_JOYAXISMOTION:
        //printf("SDL_JOYAXISMOTION: joystick: %d axis: %d value: %d\n",
        //       event.jaxis.which, event.jaxis.axis, event.jaxis.value);
        break;

      case SDL_JOYBUTTONDOWN:
        //printf("SDL_JOYBUTTONDOWN: joystick: %d button: %d state: %d\n",
        //       event.jbutton.which, event.jbutton.button, event.jbutton.state);
        break;

      case SDL_JOYBUTTONUP:
        //printf("SDL_JOYBUTTONUP: joystick: %d button: %d state: %d\n",
        //       event.jbutton.which, event.jbutton.button, event.jbutton.state);
        break;

      case SDL_JOYHATMOTION:
        //printf("SDL_JOYHATMOTION: joystick: %d hat: %d value: %d\n",
        //       event.jhat.which, event.jhat.hat, event.jhat.value);
        break;

      case SDL_JOYBALLMOTION:
        //printf("SDL_JOYBALLMOTION: joystick: %d ball: %d x: %d y: %d\n",
        //       event.jball.which, event.jball.ball, event.jball.xrel, event.jball.yrel);
        break;

      case SDL_JOYDEVICEADDED:
        //printf("SDL_JOYDEVICEADDED which:%d\n", event.jdevice.which);
        break;

      case SDL_JOYDEVICEREMOVED:
        //printf("SDL_JOYDEVICEREMOVED which:%d\n", event.jdevice.which);
        break;

      case SDL_CONTROLLERAXISMOTION:
        printf("SDL_CONTROLLERAXISMOTION controller: %d axis: %-12s value: %d\n",
               event.caxis.which,
          SDL_GameControllerGetStringForAxis(event.caxis.axis),
               event.caxis.value);
        break;

      case SDL_CONTROLLERBUTTONDOWN:
        printf("SDL_CONTROLLERBUTTONDOWN controller: %d button: %s state: %d\n",
               event.cbutton.which,
               SDL_GameControllerGetStringForButton(event.cbutton.button),
               event.cbutton.state);
        break;

      case SDL_CONTROLLERBUTTONUP:
        printf("SDL_CONTROLLERBUTTONUP   controller: %d button: %s state: %d\n",
               event.cbutton.which,
               SDL_GameControllerGetStringForButton(event.cbutton.button),
               event.cbutton.state);
        break;

      case SDL_CONTROLLERDEVICEADDED:
        printf("SDL_CONTROLLERDEVICEADDED which:%d\n", event.cdevice.which);
        break;

      case SDL_CONTROLLERDEVICEREMOVED:
        printf("SDL_CONTROLLERDEVICEREMOVED which:%d\n",  event.cdevice.which);
        break;

      case SDL_CONTROLLERDEVICEREMAPPED:
        printf("SDL_CONTROLLERDEVICEREMAPPED which:%d\n", event.cdevice.which);
        break;

      case SDL_QUIT:
        quit = 1;
        printf("Recieved interrupt, exiting\n");
        break;

      case SDL_KEYMAPCHANGED:
        break;

      default:
        fprintf(stderr, "Error: Unhandled event type: %#x\n", event.type);
        break;
    }
  }
}
#endif

static void test_gamecontroller_state(SDL_GameController* gamepad)
{
  assert(gamepad);

  printf("Entering gamecontroller test loop, press Ctrl-c to exit\n");
  int quit = 0;
  SDL_Event event;

  while(!quit && SDL_WaitEvent(&event))
  {
    switch(event.type)
    {
      case SDL_QUIT:
        quit = 1;
        printf("Recieved interrupt, exiting\n");
        break;
    }

    for(int btn = 0; btn < SDL_CONTROLLER_BUTTON_MAX; ++btn)
    {
      printf("%s:%d ",
             SDL_GameControllerGetStringForButton(btn),
             SDL_GameControllerGetButton(gamepad, btn));
    }

    for(int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis)
    {
      printf("%s:%6d ",
             SDL_GameControllerGetStringForAxis(axis),
             SDL_GameControllerGetAxis(gamepad, axis));
    }

    printf("\n");
  }
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

    while(!quit && SDL_WaitEvent(&event))
    {
      switch(event.type)
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
          printf("SDL_CONTROLLERDEVICEREMOVED which:%d\n",  event.cdevice.which);
          break;

        case SDL_CONTROLLERDEVICEREMAPPED:
          printf("SDL_CONTROLLERDEVICEREMAPPED which:%d\n", event.cdevice.which);
          break;

        case SDL_QUIT:
          quit = 1;
          printf("Recieved interrupt, exiting\n");
          break;

        default:
          fprintf(stderr, "Error: Unhandled event type: %d\n", event.type);
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
#if SDL_VERSION_ATLEAST(2,0,18)
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
  if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) < 0)
  {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    exit(1);
  }
  else
  {
    atexit(SDL_Quit);

    {
      int ret = SDL_GameControllerAddMappingsFromFile(SDL2_JSTEST_DATADIR "/gamecontrollerdb.txt");
      if (ret < 0) {
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
