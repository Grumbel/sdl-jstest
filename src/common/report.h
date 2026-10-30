// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/* Startup report (gamepkgstd LOGGING.md) and the platform bits it needs:
   one `topic: details` line per item on stderr. The SDL-specific lines are
   written by each binary; this file has what does not depend on SDL. */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void report_line(const char* topic, const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

/* "<program>: <version>, <build type>, <arch>, <compiler>" */
void report_program(const char* program, const char* version);

/* "system: <SDL platform>, <OS / kernel>, <cpus>, <ram>"; cpus / ram_mb
   below 0 are looked up here, sdl_platform may be NULL (SDL 1.2). */
void report_system(const char* sdl_platform, int cpus, int ram_mb);

/* "env:" lines for SDL_* variables (values cut), LD_PRELOAD and
   LD_LIBRARY_PATH. */
void report_env(void);

/* "ncurses: <version>" when built with the terminal UI. */
void report_ncurses(void);

/* The file a loaded shared library was mapped from (/proc/self/maps; the
   first path whose file name starts with `prefix`), or NULL. */
const char* report_mapped_library(const char* prefix, char* buf, size_t size);

/* Where a controller mapping came from: SDL_GAMECONTROLLERCONFIG,
   SDL_GAMECONTROLLERCONFIG_FILE, the database the program loaded
   (`db_text`, named `db_label`), else "SDL's built-in database". Matched by
   the start of the bindings after "guid,name,", since SDL may rewrite the
   GUID (CRC) and name. */
const char* report_mapping_source(const char* mapping, const char* db_text, const char* db_label);

/* Directory of the running executable with a trailing separator, or NULL
   (Android, or the platform cannot tell). */
const char* platform_exe_dir(void);

typedef struct DataDir {
    char path[4096];           /* empty: none found */
    const char* source;        /* "--data-dir", "next to the executable", ... */
} DataDir;

/* gamepkgstd DIR-3: --data-dir, then relative to the executable
   (../share/sdl-jstest, Windows data/), then the compiled-in directory.
   `exe_dir` may be NULL. On Android the data are APK assets and `path`
   stays empty with source "APK assets". Never the working directory. */
void platform_find_data_dir(DataDir* out, const char* option, const char* exe_dir,
                            const char* compiled_in);

/* path + "/" + name into buf; returns buf, or NULL when no data directory
   was found. On Android the plain name, which SDL resolves to the APK
   asset. */
const char* platform_data_file(const DataDir* dir, const char* name, char* buf, size_t size);

/* setenv() for strict C99 sources (SDL 1.2 has no SDL_setenv). */
void platform_setenv(const char* name, const char* value);

/* Windows: the programs are GUI executables (no console window opens,
   INT-12); attach to the console of the shell they were started from and
   reopen stdout / stderr there, so --help, --list and the startup report
   still show up. A no-op elsewhere. Call first thing in main(). */
void platform_attach_console(void);

/* Android: stdout and stderr have no reader there; send them to logcat
   (tag `tag`, one entry per line) through a pipe and a reader thread, so
   `adb logcat -s sdl3-jstest` shows the startup report. A no-op
   elsewhere. Call first thing in main(). */
void platform_log_to_logcat(const char* tag);

/* `--show-paths` output on stdout. */
void platform_print_paths(const DataDir* dir);

#ifdef __cplusplus
}
#endif
