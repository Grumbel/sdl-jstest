// SPDX-FileCopyrightText: 2009-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-FileCopyrightText: 2017 Jaime Moreira <jmoreirag@gmail.com>
// SPDX-FileCopyrightText: 2018 Anthony J. Bentley <anthony@anjbe.name>
// SPDX-FileCopyrightText: 2023 orbea <orbea@riseup.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <assert.h>
#include <curses.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "ui/test_ui.h"

static const char* sdl_event_name(Uint32 type)
{
  switch (type) {
#define E(x) case x: return #x
    E(SDL_QUIT);
    E(SDL_JOYAXISMOTION);
    E(SDL_JOYBALLMOTION);
    E(SDL_JOYHATMOTION);
    E(SDL_JOYBUTTONDOWN);
    E(SDL_JOYBUTTONUP);
    E(SDL_VIDEORESIZE);
    E(SDL_VIDEOEXPOSE);
    E(SDL_ACTIVEEVENT);
    E(SDL_KEYDOWN);
    E(SDL_KEYUP);
    E(SDL_MOUSEMOTION);
    E(SDL_MOUSEBUTTONDOWN);
    E(SDL_MOUSEBUTTONUP);
#undef E
  default: {
    static char buf[48];
    snprintf(buf, sizeof(buf), "UNKNOWN(%u)", (unsigned)type);
    return buf;
  }
  }
}


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#  include <windows.h>
#endif

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

static void print_joystick_info(int joy_idx, SDL_Joystick* joy)
{
  printf("Joystick Name:     '%s'\n", SDL_JoystickName(joy_idx));
  printf("Joystick Number:   %2d\n", joy_idx);
  printf("Number of Axes:    %2d\n", SDL_JoystickNumAxes(joy));
  printf("Number of Buttons: %2d\n", SDL_JoystickNumButtons(joy));
  printf("Number of Hats:    %2d\n", SDL_JoystickNumHats(joy));
  printf("Number of Balls:   %2d\n", SDL_JoystickNumBalls(joy));
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
  printf("  --help             Print this help\n");
  printf("  --version          Print version number and exit\n");
  printf("  --list             Search for available joysticks and list their properties\n");
  printf("  --test  JOYNUM     Display a graphical representation of the current joystick state\n");
  printf("  --event JOYNUM     Display the events that are received from the joystick\n");
  printf("\n");
  printf("Examples:\n");
  printf("  %s --list\n", prg);
  printf("  %s --test 1\n", prg);
}


static void interactive_menu(void)
{
  for (;;)
  {
    int njoy = SDL_NumJoysticks();
    char** labels = calloc((size_t)(njoy > 0 ? njoy : 1), sizeof(char*));
    for (int i = 0; i < njoy; ++i)
    {
      char buf[256];
      const char* name = SDL_JoystickName(i);
      snprintf(buf, sizeof(buf), "[joy]  %s", name ? name : "(unknown)");
      labels[i] = malloc(strlen(buf) + 1);
      if (labels[i])
        memcpy(labels[i], buf, strlen(buf) + 1);
    }

    TestUI ui;
    test_ui_init(&ui);
    int sel = test_ui_menu_select(&ui, "Select joystick",
                                  (const char* const*)labels, njoy);
    test_ui_shutdown(&ui);

    for (int i = 0; i < njoy; ++i)
      free(labels[i]);
    free(labels);

    if (sel < 0)
      return;

    /* Open and run test for selected index (inline path via --test logic) */
    {
      char idxbuf[16];
      snprintf(idxbuf, sizeof(idxbuf), "%d", sel);
      /* Re-enter by calling the same open logic: spawn test by setting up args-like call */
    }
    /* Direct test body: use existing --test path via function extraction - call joystick open loop */
    {
      SDL_Joystick* joy = SDL_JoystickOpen(sel);
      if (!joy)
      {
        fprintf(stderr, "Unable to open joystick %d\n", sel);
        continue;
      }
      int num_axes = SDL_JoystickNumAxes(joy);
      int num_buttons = SDL_JoystickNumButtons(joy);
      int num_hats = SDL_JoystickNumHats(joy);
      int num_balls = SDL_JoystickNumBalls(joy);
      Sint16* axes = calloc((size_t)num_axes, sizeof(Sint16));
      Uint8* buttons = calloc((size_t)num_buttons, sizeof(Uint8));
      Uint8* hats = calloc((size_t)num_hats, sizeof(Uint8));
      Sint16* balls = calloc((size_t)num_balls, 2 * sizeof(Sint16));
      for (int i = 0; i < num_axes; ++i) axes[i] = SDL_JoystickGetAxis(joy, i);
      for (int i = 0; i < num_buttons; ++i) buttons[i] = SDL_JoystickGetButton(joy, i);
      for (int i = 0; i < num_hats; ++i) hats[i] = SDL_JoystickGetHat(joy, i);

      TestUI ui;
      test_ui_init(&ui);
      test_ui_log(&ui, "Opened joystick %d '%s'", sel,
                  SDL_JoystickName(sel) ? SDL_JoystickName(sel) : "?");
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
              break;
            case SDL_QUIT:
              quit = 1;
              break;
            default:
              test_ui_log(&ui, "%s", sdl_event_name(event.type));
              break;
          }
        }
        test_ui_draw_joystick(&ui, SDL_JoystickName(sel), sel,
                              axes, num_axes, buttons, num_buttons, NULL,
                              NULL, 0, hats, num_hats, balls, num_balls);
        test_ui_refresh(&ui);
        {
          TestUIAction act = test_ui_poll_action(&ui);
          if (act == TEST_UI_QUIT)
          {
            test_ui_shutdown(&ui);
            free(balls); free(hats); free(buttons); free(axes);
            SDL_JoystickClose(joy);
            exit(EXIT_SUCCESS);
          }
          if (act == TEST_UI_BACK)
            quit = 1;
        }
      }
      test_ui_shutdown(&ui);
      free(balls); free(hats); free(buttons); free(axes);
      SDL_JoystickClose(joy);
    }
  }
}


int main(int argc, char** argv)
{
#ifdef _WIN32
  FILE* fp;

  AllocConsole();
  freopen_s(&fp, "CONIN$", "r", stdin);
  freopen_s(&fp, "CONOUT$", "w", stdout);
  freopen_s(&fp, "CONOUT$", "w", stderr);
#endif

  /* menu when no args */

  // FIXME: We don't need video, but without it SDL will fail to work in SDL_WaitEvent()
  if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
  {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    exit(1);
  }
  else
  {
    atexit(SDL_Quit);

    if (argc == 1)
  {
    interactive_menu();
  }
  else if (argc == 2 && (strcmp(argv[1], "--help") == 0 ||
                      strcmp(argv[1], "-h") == 0))
    {
      print_help(argv[0]);
    }
    if (argc == 2 && (strcmp(argv[1], "--version") == 0))
    {
      printf("sdl1-jstest %s\n", SDL_JSTEST_VERSION);
      exit(EXIT_SUCCESS);
    }
    else if (argc == 2 && (strcmp(argv[1], "--list") == 0 ||
                           (strcmp(argv[1], "-l") == 0)))
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
            print_joystick_info(joy_idx, joy);
            SDL_JoystickClose(joy);
          }
        }
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


      SDL_Joystick* joy = SDL_JoystickOpen(joy_idx);
      if (!joy)
      {
        fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
      }
      else
      {
        int num_axes    = SDL_JoystickNumAxes(joy);
        int num_buttons = SDL_JoystickNumButtons(joy);
        int num_hats    = SDL_JoystickNumHats(joy);
        int num_balls   = SDL_JoystickNumBalls(joy);
        if (num_axes < 0 || num_buttons < 0 || num_hats < 0 || num_balls < 0) {
          fprintf(stderr, "Unable to query joystick layout: %s\n", SDL_GetError());
          exit(EXIT_FAILURE);
        }

        Sint16* axes    = calloc((size_t)num_axes,    sizeof(Sint16));
        Uint8*  buttons = calloc((size_t)num_buttons, sizeof(Uint8));
        Uint8*  hats    = calloc((size_t)num_hats,    sizeof(Uint8));
        Sint16* balls   = calloc((size_t)num_balls,   2*sizeof(Sint16));

        for (int i = 0; i < num_axes; ++i)
          axes[i] = SDL_JoystickGetAxis(joy, i);
        for (int i = 0; i < num_buttons; ++i)
          buttons[i] = SDL_JoystickGetButton(joy, i);
        for (int i = 0; i < num_hats; ++i)
          hats[i] = SDL_JoystickGetHat(joy, i);

        TestUI ui;
        test_ui_init(&ui);
        test_ui_log(&ui, "Opened joystick %d '%s'", joy_idx,
                    SDL_JoystickName(joy_idx) ? SDL_JoystickName(joy_idx) : "?");

        int quit = 0;
        SDL_Event event;
        while(!quit)
        {
          SDL_Delay(10);
          while (SDL_PollEvent(&event)) {
            switch(event.type)
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
                if (event.jball.ball < num_balls) {
                  balls[2*event.jball.ball + 0] = event.jball.xrel;
                  balls[2*event.jball.ball + 1] = event.jball.yrel;
                }
                test_ui_log(&ui, "BALL   joy=%d ball=%d dx=%d dy=%d",
                            event.jball.which, event.jball.ball,
                            event.jball.xrel, event.jball.yrel);
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
            test_ui_draw_joystick(&ui, SDL_JoystickName(joy_idx), joy_idx,
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
        SDL_JoystickClose(joy);
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

      SDL_Joystick* joy = SDL_JoystickOpen(joy_idx);
      if (!joy)
      {
        fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
      }
      else
      {
        print_joystick_info(joy_idx, joy);

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
      fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
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
