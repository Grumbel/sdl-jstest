// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/cli.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum OptArg { ARG_NONE, ARG_REQUIRED, ARG_JOYNUM_OPTIONAL } OptArg;

typedef struct OptSpec {
    char short_name;     /* 0: long only */
    const char* long_name;
    OptArg arg;
    int id;
    unsigned feature;    /* 0: always available */
} OptSpec;

enum {
    OPT_HELP, OPT_VERSION, OPT_SHOW_PATHS, OPT_LIST, OPT_TEST, OPT_GAMECONTROLLER,
    OPT_EVENT, OPT_RUMBLE, OPT_GUI, OPT_DATA_DIR, OPT_FULLSCREEN, OPT_WINDOWED,
    OPT_GEOMETRY
};

static const OptSpec kOptions[] = {
    { 'h', "help", ARG_NONE, OPT_HELP, 0 },
    { 'v', "version", ARG_NONE, OPT_VERSION, 0 },
    { 0, "show-paths", ARG_NONE, OPT_SHOW_PATHS, 0 },
    { 'l', "list", ARG_NONE, OPT_LIST, 0 },
    { 't', "test", ARG_REQUIRED, OPT_TEST, 0 },
    { 'g', "gamecontroller", ARG_REQUIRED, OPT_GAMECONTROLLER, JSTEST_CLI_GAMECONTROLLER },
    { 'e', "event", ARG_REQUIRED, OPT_EVENT, 0 },
    { 'r', "rumble", ARG_REQUIRED, OPT_RUMBLE, JSTEST_CLI_RUMBLE },
    { 'G', "gui", ARG_JOYNUM_OPTIONAL, OPT_GUI, 0 },
    { 0, "data-dir", ARG_REQUIRED, OPT_DATA_DIR, 0 },
    { 'f', "fullscreen", ARG_NONE, OPT_FULLSCREEN, 0 },
    { 'w', "windowed", ARG_NONE, OPT_WINDOWED, 0 },
    { 0, "geometry", ARG_REQUIRED, OPT_GEOMETRY, 0 },
};

static bool parse_int(const char* str, int* val)
{
    char* end;
    errno = 0;
    long tmp = strtol(str, &end, 10);
    if (errno != 0 || end == str || *end != '\0' || tmp < INT_MIN || tmp > INT_MAX)
        return false;
    *val = (int)tmp;
    return true;
}

static bool is_joynum(const char* s)
{
    int v;
    return s && parse_int(s, &v) && v >= 0;
}

static const OptSpec* find_long(const char* name, size_t len, unsigned features)
{
    for (size_t i = 0; i < sizeof(kOptions) / sizeof(kOptions[0]); ++i) {
        const OptSpec* o = &kOptions[i];
        if ((o->feature & features) != o->feature) continue;
        if (strlen(o->long_name) == len && strncmp(o->long_name, name, len) == 0)
            return o;
    }
    return NULL;
}

static const OptSpec* find_short(char c, unsigned features)
{
    for (size_t i = 0; i < sizeof(kOptions) / sizeof(kOptions[0]); ++i) {
        const OptSpec* o = &kOptions[i];
        if ((o->feature & features) != o->feature) continue;
        if (o->short_name == c) return o;
    }
    return NULL;
}

static bool set_mode(JstestOptions* opts, JstestMode mode, const char* name)
{
    if (opts->mode != JSTEST_MODE_DEFAULT && opts->mode != mode) {
        fprintf(stderr, "error: %s cannot be combined with another mode option\n", name);
        return false;
    }
    opts->mode = mode;
    return true;
}

static bool apply(JstestOptions* opts, const OptSpec* o, const char* name, const char* value)
{
    switch (o->id) {
    case OPT_HELP: return set_mode(opts, JSTEST_MODE_HELP, name);
    case OPT_VERSION: return set_mode(opts, JSTEST_MODE_VERSION, name);
    case OPT_SHOW_PATHS: return set_mode(opts, JSTEST_MODE_SHOW_PATHS, name);
    case OPT_LIST: return set_mode(opts, JSTEST_MODE_LIST, name);
    case OPT_TEST: case OPT_GAMECONTROLLER: case OPT_EVENT: case OPT_RUMBLE: case OPT_GUI: {
        static const JstestMode modes[] = {
            [OPT_TEST] = JSTEST_MODE_TEST, [OPT_GAMECONTROLLER] = JSTEST_MODE_GAMECONTROLLER,
            [OPT_EVENT] = JSTEST_MODE_EVENT, [OPT_RUMBLE] = JSTEST_MODE_RUMBLE,
            [OPT_GUI] = JSTEST_MODE_GUI,
        };
        if (value) {
            int idx;
            if (!parse_int(value, &idx) || idx < 0) {
                fprintf(stderr, "error: %s: JOYNUM must be a number >= 0, but was '%s'\n", name, value);
                return false;
            }
            opts->joy_idx = idx;
        }
        return set_mode(opts, modes[o->id], name);
    }
    case OPT_DATA_DIR:
        if (!*value) {
            fprintf(stderr, "error: %s: empty directory\n", name);
            return false;
        }
        opts->data_dir = value;
        return true;
    case OPT_FULLSCREEN: opts->fullscreen = 1; return true;
    case OPT_WINDOWED: opts->fullscreen = 0; return true;
    case OPT_GEOMETRY: {
        int w = 0, h = 0;
        char tail;
        if (sscanf(value, "%dx%d%c", &w, &h, &tail) != 2 || w <= 0 || h <= 0) {
            fprintf(stderr, "error: %s: expected WIDTHxHEIGHT, but was '%s'\n", name, value);
            return false;
        }
        opts->width = w;
        opts->height = h;
        return true;
    }
    }
    return false;
}

bool cli_parse(int argc, char** argv, unsigned features, JstestOptions* opts)
{
    memset(opts, 0, sizeof(*opts));
    opts->mode = JSTEST_MODE_DEFAULT;
    opts->joy_idx = -1;
    opts->fullscreen = -1;

    bool options_done = false;
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (!options_done && strcmp(arg, "--") == 0) {
            options_done = true;
            continue;
        }
        if (options_done || arg[0] != '-' || arg[1] == '\0') {
            fprintf(stderr, "error: unexpected argument '%s'\n", arg);
            return false;
        }

        if (arg[1] == '-') {
            const char* name = arg + 2;
            const char* eq = strchr(name, '=');
            size_t len = eq ? (size_t)(eq - name) : strlen(name);
            const OptSpec* o = find_long(name, len, features);
            if (!o) {
                fprintf(stderr, "error: unknown option '%.*s'\n", (int)(len + 2), arg);
                return false;
            }
            const char* value = NULL;
            if (o->arg == ARG_NONE) {
                if (eq) {
                    fprintf(stderr, "error: option '--%s' takes no value\n", o->long_name);
                    return false;
                }
            } else if (eq) {
                value = eq + 1;
            } else if (o->arg == ARG_REQUIRED) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "error: option '--%s' needs a value\n", o->long_name);
                    return false;
                }
                value = argv[++i];
            } else if (is_joynum(i + 1 < argc ? argv[i + 1] : NULL)) {
                value = argv[++i];
            }
            if (!apply(opts, o, arg, value)) return false;
        } else {
            /* One or more short options; a value may follow attached or as
               the next argument. */
            for (const char* p = arg + 1; *p; ++p) {
                const OptSpec* o = find_short(*p, features);
                char name[3] = { '-', *p, '\0' };
                if (!o) {
                    fprintf(stderr, "error: unknown option '%s'\n", name);
                    return false;
                }
                const char* value = NULL;
                if (o->arg != ARG_NONE) {
                    if (p[1]) {
                        value = p + 1;
                    } else if (o->arg == ARG_REQUIRED) {
                        if (i + 1 >= argc) {
                            fprintf(stderr, "error: option '%s' needs a value\n", name);
                            return false;
                        }
                        value = argv[++i];
                    } else if (is_joynum(i + 1 < argc ? argv[i + 1] : NULL)) {
                        value = argv[++i];
                    }
                }
                if (!apply(opts, o, name, value)) return false;
                if (value) break;
            }
        }
    }
    return true;
}

void cli_print_usage(const char* prg, unsigned features, bool has_terminal_ui)
{
    printf("Usage: %s [OPTION]...\n", prg);
    printf("List available joysticks or test a joystick through SDL, the way games\n"
           "see it, instead of the raw /dev/input interface.\n");
    printf("With no mode option, opens an interactive device menu%s.\n",
           has_terminal_ui ? " in the terminal" : " in a window");
    printf("\nModes:\n");
    printf("  -l, --list               List the joysticks and their properties\n");
    if (has_terminal_ui) {
        printf("  -t, --test JOYNUM        Show the joystick's state in the terminal\n");
        if (features & JSTEST_CLI_GAMECONTROLLER)
            printf("  -g, --gamecontroller JOYNUM\n"
                   "                           Show the gamecontroller's state in the terminal\n");
    }
    printf("  -G, --gui [JOYNUM]       Show the device menu, or JOYNUM's state, in a window\n");
    printf("  -e, --event JOYNUM       Print the events received from the joystick\n");
    if (features & JSTEST_CLI_RUMBLE)
        printf("  -r, --rumble JOYNUM      Rumble the joystick for 3 seconds\n");
    printf("  -h, --help               Print this help and exit\n");
    printf("  -v, --version            Print the version and exit\n");
    printf("      --show-paths         Print the data directory and where it came from, then exit\n");
    printf("\nOptions:\n");
    printf("      --data-dir DIR       Directory with gamecontrollerdb.txt\n");
    printf("  -f, --fullscreen         Window modes: fullscreen\n");
    printf("  -w, --windowed           Window modes: windowed (default)\n");
    printf("      --geometry WxH       Window size (default 640x480)\n");
    if (features & JSTEST_CLI_GAMECONTROLLER)
        printf("\nIn the window: arrow keys, Enter and Esc; gamepad D-pad, A and B;\n"
               "touch or mouse. Leave a device view with Esc, the Back key, tapping\n"
               "'Menu', or holding the gamepad's Back / Select button for a second.\n");
    else
        printf("\nIn the window: arrow keys, Enter and Esc; mouse; on a joystick the\n"
               "stick or hat moves, pressing a button steps to the next row and\n"
               "holding one opens it. Leave a device view with Esc, clicking 'Menu',\n"
               "or holding any joystick button for two seconds.\n");
    printf("\nExamples:\n");
    printf("  %s --list\n", prg);
    if (has_terminal_ui)
        printf("  %s --test 1\n", prg);
    printf("  %s --gui\n", prg);
    printf("  %s --gui 0 --fullscreen\n", prg);
}
