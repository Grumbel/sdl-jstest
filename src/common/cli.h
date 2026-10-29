// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* What the program was asked to do; one per run. */
typedef enum JstestMode {
    JSTEST_MODE_DEFAULT = 0,   /* interactive menu: terminal, else window */
    JSTEST_MODE_HELP,
    JSTEST_MODE_VERSION,
    JSTEST_MODE_SHOW_PATHS,
    JSTEST_MODE_LIST,
    JSTEST_MODE_TEST,          /* terminal joystick view */
    JSTEST_MODE_GAMECONTROLLER,/* terminal gamecontroller view */
    JSTEST_MODE_EVENT,
    JSTEST_MODE_RUMBLE,
    JSTEST_MODE_GUI            /* window: menu, or one device with joy_idx */
} JstestMode;

/* Modes a binary offers beyond the common ones (sdl1-jstest has neither). */
enum {
    JSTEST_CLI_GAMECONTROLLER = 1 << 0,
    JSTEST_CLI_RUMBLE = 1 << 1
};

typedef struct JstestOptions {
    JstestMode mode;
    int joy_idx;               /* -1: none given */
    const char* data_dir;      /* --data-dir, or NULL */
    int fullscreen;            /* -1 default, 0 windowed, 1 fullscreen */
    int width, height;         /* --geometry, 0 = default */
} JstestOptions;

/* Parses argv (GNU style: -x, --long, --long=VALUE, --long VALUE, "--").
   Errors go to stderr; returns false then (exit status 2). */
bool cli_parse(int argc, char** argv, unsigned features, JstestOptions* opts);

/* The usage text of the options above, for --help. */
void cli_print_usage(const char* prg, unsigned features, bool has_terminal_ui);

#ifdef __cplusplus
}
#endif
