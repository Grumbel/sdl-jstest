// SPDX-FileCopyrightText: 2009-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-FileCopyrightText: 2017 Jaime Moreira <jmoreirag@gmail.com>
// SPDX-FileCopyrightText: 2018 Anthony J. Bentley <anthony@anjbe.name>
// SPDX-FileCopyrightText: 2023 orbea <orbea@riseup.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>

#include <SDL.h>

#include "common/cli.h"
#include "common/report.h"
#include "ui/graph_ui.h"
#include "ui/gui_nav.h"
#if defined(SDL_JSTEST_NCURSES)
#  include "ui/test_ui.h"
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#  include <windows.h>
#endif

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

static void list_joysticks(void)
{
    int num_joysticks = SDL_NumJoysticks();
    if (num_joysticks == 0) {
        printf("No joysticks were found\n");
        return;
    }
    printf("Found %d joystick(s)\n\n", num_joysticks);
    for (int joy_idx = 0; joy_idx < num_joysticks; ++joy_idx) {
        SDL_Joystick* joy = SDL_JoystickOpen(joy_idx);
        if (!joy) {
            fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
        } else {
            print_joystick_info(joy_idx, joy);
            SDL_JoystickClose(joy);
        }
    }
}

static void event_joystick(int joy_idx)
{
    SDL_Joystick* joy = SDL_JoystickOpen(joy_idx);
    if (!joy) {
        fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
        return;
    }
    print_joystick_info(joy_idx, joy);

    printf("Entering joystick test loop, press Ctrl-c to exit\n");
    /* Polled, since SDL_WaitEvent would not notice the signal handler. */
    while (!g_app_quit) {
        SDL_Event event;
        if (!SDL_PollEvent(&event)) {
            SDL_Delay(10);
            continue;
        }
        switch (event.type) {
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
            g_app_quit = 1;
            printf("Recieved interrupt, exiting\n");
            break;
        default:
            printf("%s\n", sdl_event_name(event.type));
            break;
        }
    }
    SDL_JoystickClose(joy);
}

#if defined(SDL_JSTEST_NCURSES)
/* Returns true when the user asked to leave the program (Ctrl-C). */
static bool test_joystick(int joy_idx)
{
    SDL_Joystick* joy = SDL_JoystickOpen(joy_idx);
    if (!joy) {
        fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
        return false;
    }
    int num_axes    = SDL_JoystickNumAxes(joy);
    int num_buttons = SDL_JoystickNumButtons(joy);
    int num_hats    = SDL_JoystickNumHats(joy);
    int num_balls   = SDL_JoystickNumBalls(joy);
    if (num_axes < 0 || num_buttons < 0 || num_hats < 0 || num_balls < 0) {
        fprintf(stderr, "Unable to query joystick layout: %s\n", SDL_GetError());
        SDL_JoystickClose(joy);
        return false;
    }

    Sint16* axes    = calloc((size_t)num_axes + 1,    sizeof(Sint16));
    Uint8*  buttons = calloc((size_t)num_buttons + 1, sizeof(Uint8));
    Uint8*  hats    = calloc((size_t)num_hats + 1,    sizeof(Uint8));
    Sint16* balls   = calloc((size_t)num_balls + 1,   2*sizeof(Sint16));

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

    bool quit = false, exit_app = false;
    while (!quit && !exit_app && !g_app_quit) {
        SDL_Event event;
        SDL_Delay(10);
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
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
                if (event.jball.ball < num_balls) {
                    balls[2*event.jball.ball + 0] = event.jball.xrel;
                    balls[2*event.jball.ball + 1] = event.jball.yrel;
                }
                test_ui_log(&ui, "%s which=%d ball=%d dx=%d dy=%d",
                            sdl_event_name(event.type), event.jball.which, event.jball.ball,
                            event.jball.xrel, event.jball.yrel);
                break;
            case SDL_QUIT:
                exit_app = true;
                break;
            default:
                test_ui_log(&ui, "%s", sdl_event_name(event.type));
                break;
            }
        }

        test_ui_draw_joystick(&ui, SDL_JoystickName(joy_idx), joy_idx,
                              axes, num_axes, buttons, num_buttons, NULL,
                              NULL, 0, hats, num_hats, balls, num_balls);
        test_ui_refresh(&ui);
        TestUIAction act = test_ui_poll_action(&ui);
        if (act == TEST_UI_QUIT)
            exit_app = true;
        else if (act == TEST_UI_BACK)
            quit = true;
    }

    test_ui_shutdown(&ui);
    free(balls);
    free(hats);
    free(buttons);
    free(axes);
    SDL_JoystickClose(joy);
    return exit_app || g_app_quit;
}

static void interactive_menu(void)
{
    while (!g_app_quit) {
        int njoy = SDL_NumJoysticks();
        char** labels = calloc((size_t)(njoy > 0 ? njoy : 1), sizeof(char*));
        for (int i = 0; i < njoy; ++i) {
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

        if (sel < 0 || test_joystick(sel))
            return;
    }
}
#endif /* SDL_JSTEST_NCURSES */

/* ---- Window UI ----
   One video surface for the whole session (on a framebuffer console it is
   the screen): the 640x480 GraphUI buffer is scaled into it in software,
   aspect ratio kept. SDL 1.2 has no button names, so joysticks navigate
   by press (next row) and hold (open, or leave a device view). */

typedef struct Gui {
    SDL_Surface* screen;
    Uint32 video_flags;
    GraphUI ui;
    /* Where the buffer lands on the screen. */
    int dst_x, dst_y, dst_w, dst_h;
    int* xmap;                   /* screen column → buffer column */
    /* Every joystick stays open: SDL 1.2 only reports opened ones. */
    SDL_Joystick* joys[16];
    GuiHold hold;                /* any joystick button held */
    int buttons_down;
    bool armed;                  /* a button went down in the menu */
    GuiStick stick;
} Gui;

static void gui_layout(Gui* gui)
{
    int sw = gui->screen->w, sh = gui->screen->h;
    /* Largest size of the buffer's aspect ratio that fits. */
    if ((long)sw * gui->ui.h <= (long)sh * gui->ui.w) {
        gui->dst_w = sw;
        gui->dst_h = (int)((long)sw * gui->ui.h / gui->ui.w);
    } else {
        gui->dst_h = sh;
        gui->dst_w = (int)((long)sh * gui->ui.w / gui->ui.h);
    }
    if (gui->dst_w < 1) gui->dst_w = 1;
    if (gui->dst_h < 1) gui->dst_h = 1;
    gui->dst_x = (sw - gui->dst_w) / 2;
    gui->dst_y = (sh - gui->dst_h) / 2;
    free(gui->xmap);
    gui->xmap = malloc((size_t)gui->dst_w * sizeof(int));
    for (int x = 0; x < gui->dst_w; ++x)
        gui->xmap[x] = (int)((long)x * gui->ui.w / gui->dst_w);
}

static bool gui_set_mode(Gui* gui, int w, int h)
{
    gui->screen = SDL_SetVideoMode(w, h, 0, gui->video_flags);
    if (!gui->screen) {
        fprintf(stderr, "error: SDL_SetVideoMode %dx%d: %s\n", w, h, SDL_GetError());
        return false;
    }
    gui_layout(gui);
    return true;
}

static void report_video(Gui* gui)
{
    char driver[64] = "?";
    SDL_VideoDriverName(driver, sizeof(driver));
    const SDL_VideoInfo* info = SDL_GetVideoInfo();
    const SDL_PixelFormat* fmt = gui->screen->format;
    report_line("video", "%s, display %dx%d, surface %dx%d %d bpp (R %08x G %08x B %08x), %s%s",
                driver, info ? info->current_w : -1, info ? info->current_h : -1,
                gui->screen->w, gui->screen->h, fmt->BitsPerPixel,
                (unsigned)fmt->Rmask, (unsigned)fmt->Gmask, (unsigned)fmt->Bmask,
                (gui->screen->flags & SDL_HWSURFACE) ? "hardware surface" : "software surface",
                (gui->screen->flags & SDL_FULLSCREEN) ? ", fullscreen" : "");
}

static bool gui_open(Gui* gui, const JstestOptions* opts)
{
    memset(gui, 0, sizeof(*gui));
    if (!graph_ui_init(&gui->ui, GRAPH_UI_DEFAULT_W, GRAPH_UI_DEFAULT_H)) return false;
    SDL_WM_SetCaption("sdl1-jstest", NULL);
    int w = opts->width > 0 ? opts->width : gui->ui.w;
    int h = opts->height > 0 ? opts->height : gui->ui.h;
    if (opts->fullscreen == 1) {
        gui->video_flags = SDL_SWSURFACE | SDL_FULLSCREEN;
        /* 0x0: the current display mode (SDL >= 1.2.10). */
        if (opts->width <= 0) w = h = 0;
    } else {
        gui->video_flags = SDL_SWSURFACE | SDL_RESIZABLE;
    }
    if (!gui_set_mode(gui, w, h)) {
        graph_ui_shutdown(&gui->ui);
        return false;
    }
    report_video(gui);
    for (int i = 0; i < SDL_NumJoysticks() && i < (int)(sizeof(gui->joys) / sizeof(gui->joys[0])); ++i)
        gui->joys[i] = SDL_JoystickOpen(i);
    return true;
}

static void gui_close(Gui* gui)
{
    for (size_t i = 0; i < sizeof(gui->joys) / sizeof(gui->joys[0]); ++i)
        if (gui->joys[i]) SDL_JoystickClose(gui->joys[i]);
    free(gui->xmap);
    graph_ui_shutdown(&gui->ui);
    memset(gui, 0, sizeof(*gui));
}

static void gui_present(Gui* gui)
{
    SDL_Surface* s = gui->screen;
    const SDL_PixelFormat* fmt = s->format;
    if (gui->dst_w != s->w || gui->dst_h != s->h)
        SDL_FillRect(s, NULL, SDL_MapRGB(s->format, 0, 0, 0));
    if (SDL_MUSTLOCK(s)) SDL_LockSurface(s);
    bool argb = fmt->BytesPerPixel == 4 && fmt->Rmask == 0xFF0000u &&
                fmt->Gmask == 0xFF00u && fmt->Bmask == 0xFFu;
    for (int y = 0; y < gui->dst_h; ++y) {
        const uint32_t* src = gui->ui.pixels + (long)y * gui->ui.h / gui->dst_h * gui->ui.w;
        Uint8* row = (Uint8*)s->pixels + (gui->dst_y + y) * s->pitch + gui->dst_x * fmt->BytesPerPixel;
        if (argb) {
            Uint32* dst = (Uint32*)(void*)row;
            if (gui->dst_w == gui->ui.w)
                memcpy(dst, src, (size_t)gui->ui.w * 4);
            else
                for (int x = 0; x < gui->dst_w; ++x) dst[x] = src[gui->xmap[x]];
            continue;
        }
        for (int x = 0; x < gui->dst_w; ++x) {
            uint32_t c = src[gui->xmap[x]];
            Uint32 p = SDL_MapRGB(s->format, (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c);
            Uint8* dst = row + x * fmt->BytesPerPixel;
            switch (fmt->BytesPerPixel) {
            case 1: *dst = (Uint8)p; break;
            case 2: *(Uint16*)(void*)dst = (Uint16)p; break;
            case 3:
                if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {
                    dst[0] = (Uint8)(p); dst[1] = (Uint8)(p >> 8); dst[2] = (Uint8)(p >> 16);
                } else {
                    dst[2] = (Uint8)(p); dst[1] = (Uint8)(p >> 8); dst[0] = (Uint8)(p >> 16);
                }
                break;
            case 4: *(Uint32*)(void*)dst = p; break;
            }
        }
    }
    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);
    SDL_Flip(s);
    SDL_Delay(10);
}

/* Screen → buffer coordinates; false outside the buffer. */
static bool gui_to_buffer(const Gui* gui, int sx, int sy, int* bx, int* by)
{
    if (sx < gui->dst_x || sy < gui->dst_y ||
        sx >= gui->dst_x + gui->dst_w || sy >= gui->dst_y + gui->dst_h)
        return false;
    *bx = (int)((long)(sx - gui->dst_x) * gui->ui.w / gui->dst_w);
    *by = (int)((long)(sy - gui->dst_y) * gui->ui.h / gui->dst_h);
    return true;
}

/* Event → navigation. Window events (resize) are handled here too. */
static GuiAction gui_action(Gui* gui, const SDL_Event* ev, bool in_menu, int* pick)
{
    switch (ev->type) {
    case SDL_QUIT:
        return GUI_QUIT;
    case SDL_VIDEORESIZE:
        if (!gui_set_mode(gui, ev->resize.w, ev->resize.h)) return GUI_QUIT;
        return GUI_NONE;
    case SDL_KEYDOWN: {
        SDLKey k = ev->key.keysym.sym;
        if (k == SDLK_c && (ev->key.keysym.mod & KMOD_CTRL)) return GUI_QUIT;
        if (k == SDLK_ESCAPE || k == SDLK_q) return GUI_BACK;
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
    case SDL_MOUSEBUTTONDOWN: {
        int x, y;
        if (ev->button.button != SDL_BUTTON_LEFT ||
            !gui_to_buffer(gui, ev->button.x, ev->button.y, &x, &y))
            return GUI_NONE;
        if (in_menu) {
            *pick = graph_ui_menu_hit(&gui->ui, x, y);
            return *pick >= 0 ? GUI_PICK : GUI_NONE;
        }
        return graph_ui_back_hit(&gui->ui, x, y) ? GUI_BACK : GUI_NONE;
    }
    case SDL_JOYBUTTONDOWN:
        ++gui->buttons_down;
        gui_hold_set(&gui->hold, true, SDL_GetTicks());
        if (in_menu) gui->armed = true;
        return GUI_NONE;
    case SDL_JOYBUTTONUP: {
        if (gui->buttons_down > 0) --gui->buttons_down;
        bool step = in_menu && gui->armed && gui->buttons_down == 0;
        if (gui->buttons_down == 0) {
            gui_hold_set(&gui->hold, false, SDL_GetTicks());
            gui->armed = false;
        }
        return step ? GUI_DOWN : GUI_NONE;
    }
    case SDL_JOYHATMOTION:
        if (!in_menu) return GUI_NONE;
        if (ev->jhat.value & SDL_HAT_UP) return GUI_UP;
        if (ev->jhat.value & SDL_HAT_DOWN) return GUI_DOWN;
        return GUI_NONE;
    case SDL_JOYAXISMOTION:
        if (in_menu && ev->jaxis.axis == 1)
            return gui_stick_axis(&gui->stick, ev->jaxis.value);
        return GUI_NONE;
    default:
        return GUI_NONE;
    }
}

static void gui_reset_input(Gui* gui)
{
    memset(&gui->hold, 0, sizeof(gui->hold));
    memset(&gui->stick, 0, sizeof(gui->stick));
    gui->buttons_down = 0;
    gui->armed = false;
    gui->ui.hold_progress = 0.0f;
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
    for (int i = 0; i < num_axes; ++i) axes[i] = SDL_JoystickGetAxis(joy, i);
    for (int i = 0; i < num_buttons; ++i) buttons[i] = SDL_JoystickGetButton(joy, i);
    for (int i = 0; i < num_hats; ++i) hats[i] = SDL_JoystickGetHat(joy, i);

    const char* name = SDL_JoystickName(joy_idx);
    char title[160];
    snprintf(title, sizeof(title), "sdl1-jstest - %s", name ? name : "joystick");
    SDL_WM_SetCaption(title, NULL);
    gui_reset_input(gui);
    gui->ui.hint = "Esc / click Menu / hold any button 2 s: menu";
    gui->ui.log_len = gui->ui.log_head = 0;
    graph_ui_log(&gui->ui, "Opened joystick %d '%s'", joy_idx, name ? name : "?");

    GuiAction result = GUI_NONE;
    while (result == GUI_NONE && !g_app_quit) {
        SDL_Event event;
        while (result == GUI_NONE && SDL_PollEvent(&event)) {
            int pick = -1;
            /* Only the tested joystick's buttons count towards the hold. */
            bool other = (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP) &&
                         event.jbutton.which != joy_idx;
            GuiAction act = other ? GUI_NONE : gui_action(gui, &event, false, &pick);
            if (act == GUI_BACK || act == GUI_QUIT) {
                result = act;
                break;
            }
            switch (event.type) {
            case SDL_JOYAXISMOTION:
                if (event.jaxis.which != joy_idx) break;
                if (event.jaxis.axis < num_axes) axes[event.jaxis.axis] = event.jaxis.value;
                graph_ui_log(&gui->ui, "%s axis=%d value=%+d", sdl_event_name(event.type),
                    event.jaxis.axis, event.jaxis.value);
                break;
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                if (event.jbutton.which != joy_idx) break;
                if (event.jbutton.button < num_buttons) buttons[event.jbutton.button] = event.jbutton.state;
                graph_ui_log(&gui->ui, "%s button=%d %s", sdl_event_name(event.type),
                    event.jbutton.button, event.jbutton.state ? "DOWN" : "UP");
                break;
            case SDL_JOYHATMOTION:
                if (event.jhat.which != joy_idx) break;
                if (event.jhat.hat < num_hats) hats[event.jhat.hat] = event.jhat.value;
                graph_ui_log(&gui->ui, "%s hat=%d value=0x%02x", sdl_event_name(event.type),
                    event.jhat.hat, event.jhat.value);
                break;
            case SDL_JOYBALLMOTION:
                if (event.jball.which != joy_idx) break;
                if (event.jball.ball < num_balls) {
                    balls[2*event.jball.ball] = event.jball.xrel;
                    balls[2*event.jball.ball+1] = event.jball.yrel;
                }
                graph_ui_log(&gui->ui, "%s ball=%d dx=%d dy=%d", sdl_event_name(event.type),
                    event.jball.ball, event.jball.xrel, event.jball.yrel);
                break;
            default:
                break;
            }
        }
        gui->ui.hold_progress = gui_hold_progress(&gui->hold, SDL_GetTicks(), GUI_HOLD_ANY_MS);
        if (result == GUI_NONE && gui->ui.hold_progress >= 1.0f) result = GUI_BACK;
        graph_ui_draw_joystick(&gui->ui, name, joy_idx,
            axes, num_axes, buttons, num_buttons, NULL, NULL, 0,
            hats, num_hats, balls, num_balls);
        gui_present(gui);
    }
    free(balls); free(hats); free(buttons); free(axes);
    SDL_JoystickClose(joy);
    return g_app_quit ? GUI_QUIT : result;
}

static char* strdup_or_empty(const char* s)
{
    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

/* Device menu; the last row quits. SDL 1.2 has no hotplugging, so the list
   is fixed. Returns the joystick index, or -1 to quit. */
static int gui_menu(Gui* gui)
{
    SDL_WM_SetCaption("sdl1-jstest - select device", NULL);
    gui->ui.hint = "Up/Down  Enter: open  Esc: quit  Button: next  Hold: open";
    gui_reset_input(gui);

    int njoy = SDL_NumJoysticks();
    if (njoy < 0) njoy = 0;
    int nitems = njoy + 1;
    char** labels = calloc((size_t)nitems, sizeof(char*));
    for (int i = 0; i < njoy; ++i) {
        char buf[256];
        const char* name = SDL_JoystickName(i);
        snprintf(buf, sizeof(buf), "[joy]  %s", name ? name : "(unknown)");
        labels[i] = strdup_or_empty(buf);
    }
    labels[njoy] = strdup_or_empty(njoy ? "Quit" : "Quit   (no joysticks found)");
    gui->ui.menu_numbered = njoy;

    int choice = -2;  /* -2: stay */
    while (choice == -2) {
        if (g_app_quit) {
            choice = -1;
            break;
        }
        SDL_Event event;
        while (choice == -2 && SDL_PollEvent(&event)) {
            int pick = -1;
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
        gui->ui.hold_progress = gui->armed ?
            gui_hold_progress(&gui->hold, SDL_GetTicks(), GUI_HOLD_MS) : 0.0f;
        if (choice == -2 && gui->ui.hold_progress >= 1.0f) {
            gui->armed = false;  /* the release must not step on */
            choice = gui->ui.menu_selected;
        }
        if (choice == -2) {
            graph_ui_draw_menu(&gui->ui, "Select joystick", (const char* const*)labels, nitems);
            gui_present(gui);
        }
    }
    for (int i = 0; i < nitems; ++i) free(labels[i]);
    free(labels);
    return choice >= njoy ? -1 : choice;
}

/* --gui [JOYNUM], and the default without a terminal UI. With JOYNUM,
   leaving the device view ends the program. */
static int gui_run(const JstestOptions* opts)
{
    Gui gui;
    if (!gui_open(&gui, opts)) return EXIT_FAILURE;
    if (opts->joy_idx >= 0) {
        gui_test_joystick(&gui, opts->joy_idx);
    } else {
        for (;;) {
            int sel = gui_menu(&gui);
            if (sel < 0) break;
            if (gui_test_joystick(&gui, sel) == GUI_QUIT) break;
        }
    }
    gui_close(&gui);
    return EXIT_SUCCESS;
}

/* ---- Startup ---- */

static void report_startup(const DataDir* data_dir)
{
    char lib[512];
    const SDL_version* linked = SDL_Linked_Version();
    SDL_version compiled;
    SDL_VERSION(&compiled);
    const char* mapped = report_mapped_library("libSDL-1.2", lib, sizeof(lib));
    report_line("sdl", "%d.%d.%d (built against %d.%d.%d)%s%s", linked->major, linked->minor,
                linked->patch, compiled.major, compiled.minor, compiled.patch,
                mapped ? ", " : "", mapped ? mapped : "");
    /* sdl12-compat runs on SDL2 (which may be sdl2-compat on SDL3). */
    if ((mapped = report_mapped_library("libSDL2", lib, sizeof(lib))))
        report_line("sdl", "through %s", mapped);
    if ((mapped = report_mapped_library("libSDL3", lib, sizeof(lib))))
        report_line("sdl", "through %s", mapped);
    report_ncurses();
    report_line("data", "%s (%s)", data_dir->path[0] ? data_dir->path : "-", data_dir->source);

    char driver[64] = "none";
    SDL_VideoDriverName(driver, sizeof(driver));
    report_line("video driver", "%s", driver);

    int njoy = SDL_NumJoysticks();
    report_line("joysticks", "%d", njoy);
    for (int i = 0; i < njoy; ++i) {
        char topic[32];
        snprintf(topic, sizeof(topic), "joystick %d", i);
        const char* name = SDL_JoystickName(i);
        report_line(topic, "%s", name ? name : "?");
    }
}

/* Video is needed for the event queue, but not for the terminal modes: on a
   console without display those fall back to the dummy driver (LOG-6). */
static bool init_sdl(JstestMode mode)
{
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "error: unable to init SDL: %s\n", SDL_GetError());
        return false;
    }
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        if (mode == JSTEST_MODE_GUI) {
            fprintf(stderr, "error: unable to init SDL video: %s\n", SDL_GetError());
            return false;
        }
        fprintf(stderr, "video: unavailable (%s), using the dummy driver\n", SDL_GetError());
        platform_setenv("SDL_VIDEODRIVER", "dummy");
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
            fprintf(stderr, "error: unable to init SDL video: %s\n", SDL_GetError());
            return false;
        }
    }
    return true;
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

    JstestOptions opts;
    if (!cli_parse(argc, argv, 0, &opts)) {
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        return 2;
    }
#if defined(SDL_JSTEST_NCURSES)
    const bool has_terminal_ui = true;
#else
    const bool has_terminal_ui = false;
    if (opts.mode == JSTEST_MODE_DEFAULT || opts.mode == JSTEST_MODE_TEST)
        opts.mode = JSTEST_MODE_GUI;
#endif

    if (opts.mode == JSTEST_MODE_HELP) {
        cli_print_usage(argv[0], 0, has_terminal_ui);
        return EXIT_SUCCESS;
    }
    if (opts.mode == JSTEST_MODE_VERSION) {
        printf("sdl1-jstest %s\n", SDL_JSTEST_VERSION);
        return EXIT_SUCCESS;
    }
    DataDir data_dir;
    platform_find_data_dir(&data_dir, opts.data_dir, platform_exe_dir(), SDL1_JSTEST_DATADIR);
    if (opts.mode == JSTEST_MODE_SHOW_PATHS) {
        platform_print_paths(&data_dir);
        return EXIT_SUCCESS;
    }

    report_program("sdl1-jstest", SDL_JSTEST_VERSION);
    report_system(NULL, -1, -1);
    report_env();

    if (!init_sdl(opts.mode)) return EXIT_FAILURE;
    app_install_quit_handlers();
    report_startup(&data_dir);

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
#if defined(SDL_JSTEST_NCURSES)
    case JSTEST_MODE_DEFAULT:
        interactive_menu();
        break;
    case JSTEST_MODE_TEST:
        test_joystick(opts.joy_idx);
        break;
#endif
    default:
        break;
    }

    SDL_Quit();
    return status;
}

/* EOF */
