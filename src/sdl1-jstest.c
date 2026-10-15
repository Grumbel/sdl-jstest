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
#include "ui/graph_ui.h"
#include <stdint.h>

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


typedef struct GraphPresent1 {
    SDL_Surface* screen;
    GraphUI ui;
} GraphPresent1;

static int graph_present1_open(GraphPresent1* gp, const char* title)
{
    memset(gp, 0, sizeof(*gp));
    if (!graph_ui_init(&gp->ui, GRAPH_UI_DEFAULT_W, GRAPH_UI_DEFAULT_H)) return 0;
    SDL_WM_SetCaption(title ? title : "sdl1-jstest", NULL);
    gp->screen = SDL_SetVideoMode(gp->ui.w, gp->ui.h, 32, SDL_SWSURFACE);
    if (!gp->screen) {
        fprintf(stderr, "SDL_SetVideoMode: %s\n", SDL_GetError());
        graph_ui_shutdown(&gp->ui); return 0;
    }
    return 1;
}
static void graph_present1_frame(GraphPresent1* gp)
{
    if (SDL_MUSTLOCK(gp->screen)) SDL_LockSurface(gp->screen);
    /* Copy ARGB8888 into screen; convert if needed via per-pixel */
    int n = gp->ui.w * gp->ui.h;
    if (gp->screen->format->BytesPerPixel == 4 &&
        gp->screen->pitch == gp->ui.w * 4) {
        memcpy(gp->screen->pixels, gp->ui.pixels, (size_t)n * 4);
    } else {
        for (int y = 0; y < gp->ui.h; ++y) {
            for (int x = 0; x < gp->ui.w; ++x) {
                uint32_t c = gp->ui.pixels[y * gp->ui.w + x];
                Uint8 r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
                Uint32 p = SDL_MapRGB(gp->screen->format, r, g, b);
                Uint8* dst = (Uint8*)gp->screen->pixels + y * gp->screen->pitch + x * gp->screen->format->BytesPerPixel;
                switch (gp->screen->format->BytesPerPixel) {
                case 1: *dst = (Uint8)p; break;
                case 2: *(Uint16*)dst = (Uint16)p; break;
                case 3:
                    if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {
                        dst[0] = (Uint8)(p); dst[1] = (Uint8)(p >> 8); dst[2] = (Uint8)(p >> 16);
                    } else {
                        dst[2] = (Uint8)(p); dst[1] = (Uint8)(p >> 8); dst[0] = (Uint8)(p >> 16);
                    }
                    break;
                case 4: *(Uint32*)dst = p; break;
                }
            }
        }
    }
    if (SDL_MUSTLOCK(gp->screen)) SDL_UnlockSurface(gp->screen);
    SDL_Flip(gp->screen);
}
static void graph_present1_close(GraphPresent1* gp)
{
    graph_ui_shutdown(&gp->ui);
    memset(gp, 0, sizeof(*gp));
}
static int graph_present1_map_key(const SDL_Event* e, int* mk, TestUIAction* act)
{
    if (e->type != SDL_KEYDOWN) return 0;
    SDLKey k = e->key.keysym.sym;
    if (k == SDLK_ESCAPE || k == SDLK_q) { if (act) *act = TEST_UI_BACK; if (mk) *mk = 27; return 1; }
    if (k == SDLK_c && (e->key.keysym.mod & KMOD_CTRL)) { if (act) *act = TEST_UI_QUIT; if (mk) *mk = 27; return 1; }
    if (!mk) return 0;
    if (k == SDLK_UP || k == SDLK_k) *mk = GRAPH_UI_KEY_UP;
    else if (k == SDLK_DOWN || k == SDLK_j) *mk = GRAPH_UI_KEY_DOWN;
    else if (k == SDLK_RETURN || k == SDLK_KP_ENTER || k == SDLK_SPACE) *mk = '\n';
    else if (k >= SDLK_0 && k <= SDLK_9) *mk = '0' + (int)(k - SDLK_0);
    else return 0;
    return 1;
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
  printf("  --test  JOYNUM     Terminal (ncurses) joystick state display\n");
  printf("  --gui [JOYNUM]    SDL window joystick test (omit JOYNUM for menu)\n");
  printf("  --event JOYNUM     Display the events that are received from the joystick\n");
  printf("\n");
  printf("Examples:\n");
  printf("  %s --list\n", prg);
  printf("  %s --test 1\n", prg);
  printf("  %s --gui\n", prg);
  printf("  %s --gui 0\n", prg);
}



static void gui_test_joystick(int joy_idx)
{
    SDL_Joystick* joy = SDL_JoystickOpen(joy_idx);
    if (!joy) { fprintf(stderr, "Unable to open joystick %d\n", joy_idx); return; }
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

    GraphPresent1 gp;
    char title[128];
    snprintf(title, sizeof(title), "sdl1-jstest — joy %d", joy_idx);
    if (!graph_present1_open(&gp, title)) {
        free(balls); free(hats); free(buttons); free(axes); SDL_JoystickClose(joy); return;
    }
    graph_ui_log(&gp.ui, "Opened joystick %d '%s'", joy_idx,
                 SDL_JoystickName(joy_idx) ? SDL_JoystickName(joy_idx) : "?");

    int quit = 0;
    SDL_Event event;
    while (!quit) {
        SDL_Delay(10);
        while (SDL_PollEvent(&event)) {
            TestUIAction act = TEST_UI_CONTINUE;
            if (graph_present1_map_key(&event, NULL, &act)) {
                if (act == TEST_UI_BACK || act == TEST_UI_QUIT) quit = 1;
                continue;
            }
            switch (event.type) {
            case SDL_JOYAXISMOTION:
                if (event.jaxis.axis < num_axes) axes[event.jaxis.axis] = event.jaxis.value;
                graph_ui_log(&gp.ui, "AXIS  axis=%d value=%+d", event.jaxis.axis, event.jaxis.value);
                break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                if (event.jbutton.button < num_buttons) buttons[event.jbutton.button] = event.jbutton.state;
                graph_ui_log(&gp.ui, "BUTTON button=%d %s", event.jbutton.button,
                    event.jbutton.state ? "DOWN" : "UP");
                break;
            case SDL_JOYHATMOTION:
                if (event.jhat.hat < num_hats) hats[event.jhat.hat] = event.jhat.value;
                graph_ui_log(&gp.ui, "HAT hat=%d value=%d", event.jhat.hat, event.jhat.value);
                break;
            case SDL_JOYBALLMOTION:
                if (event.jball.ball < num_balls) {
                    balls[2*event.jball.ball]=event.jball.xrel;
                    balls[2*event.jball.ball+1]=event.jball.yrel;
                }
                break;
            case SDL_QUIT:
                quit = 1;
                break;
            default: break;
            }
        }
        const char* jname = SDL_JoystickName(joy_idx);
        graph_ui_draw_joystick(&gp.ui, jname, joy_idx,
            axes, num_axes, buttons, num_buttons, NULL, NULL, 0,
            hats, num_hats, balls, num_balls);
        graph_present1_frame(&gp);
    }
    graph_present1_close(&gp);
    free(balls); free(hats); free(buttons); free(axes);
    SDL_JoystickClose(joy);
}

static int gui_interactive_menu(void)
{
    for (;;) {
        int njoy = SDL_NumJoysticks();
        char** labels = calloc((size_t)(njoy > 0 ? njoy : 1), sizeof(char*));
        for (int i = 0; i < njoy; ++i) {
            char buf[256];
            const char* name = SDL_JoystickName(i);
            snprintf(buf, sizeof(buf), "[joy]  %s", name ? name : "(unknown)");
            labels[i] = malloc(strlen(buf)+1);
            if (labels[i]) memcpy(labels[i], buf, strlen(buf)+1);
        }
        GraphPresent1 gp;
        if (!graph_present1_open(&gp, "sdl1-jstest — select device")) {
            for (int i = 0; i < njoy; ++i) free(labels[i]);
            free(labels); return 1;
        }
        gp.ui.menu_selected = 0;
        int sel = -1, done = 0;
        while (!done) {
            graph_ui_draw_menu(&gp.ui, "Select joystick", (const char* const*)labels, njoy);
            graph_present1_frame(&gp);
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                int mkey = 0;
                if (event.type == SDL_QUIT) { sel = -1; done = 1; break; }
                if (graph_present1_map_key(&event, &mkey, NULL)) {
                    int r = graph_ui_menu_key(&gp.ui, mkey, njoy);
                    if (r == -1) { sel = -1; done = 1; }
                    else if (r >= 0) { sel = r; done = 1; }
                }
            }
            SDL_Delay(16);
        }
        graph_present1_close(&gp);
        for (int i = 0; i < njoy; ++i) free(labels[i]);
        free(labels);
        if (sel < 0) return 0;
        gui_test_joystick(sel);
    }
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

      TestUI test_ui;
      test_ui_init(&test_ui);
      test_ui_log(&test_ui, "Opened joystick %d '%s'", sel,
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
              test_ui_log(&test_ui, "%s which=%d axis=%d value=%+d",
                          sdl_event_name(event.type),
                          event.jaxis.which, event.jaxis.axis, event.jaxis.value);
              break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
              if (event.jbutton.button < num_buttons)
                buttons[event.jbutton.button] = event.jbutton.state;
              test_ui_log(&test_ui, "%s which=%d button=%d %s",
                          sdl_event_name(event.type),
                          event.jbutton.which, event.jbutton.button,
                          event.jbutton.state ? "DOWN" : "UP");
              break;
            case SDL_JOYHATMOTION:
              if (event.jhat.hat < num_hats)
                hats[event.jhat.hat] = event.jhat.value;
              test_ui_log(&test_ui, "%s which=%d hat=%d value=0x%02x",
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
              test_ui_log(&test_ui, "%s", sdl_event_name(event.type));
              break;
          }
        }
        test_ui_draw_joystick(&test_ui, SDL_JoystickName(sel), sel,
                              axes, num_axes, buttons, num_buttons, NULL,
                              NULL, 0, hats, num_hats, balls, num_balls);
        test_ui_refresh(&test_ui);
        {
          TestUIAction act = test_ui_poll_action(&test_ui);
          if (act == TEST_UI_QUIT)
          {
            test_ui_shutdown(&test_ui);
            free(balls); free(hats); free(buttons); free(axes);
            SDL_JoystickClose(joy);
            exit(EXIT_SUCCESS);
          }
          if (act == TEST_UI_BACK)
            quit = 1;
        }
      }
      test_ui_shutdown(&test_ui);
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
