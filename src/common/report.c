// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#if !defined(_WIN32)
#  define _GNU_SOURCE
#endif

#include "common/report.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <sys/utsname.h>
#  include <unistd.h>
#endif
#if defined(__ANDROID__)
#  include <sys/system_properties.h>
#endif
#if defined(SDL_JSTEST_NCURSES)
#  include <curses.h>
#endif

#if !defined(_WIN32)
extern char** environ;
#endif

#ifndef SDL_JSTEST_BUILD_TYPE
#  define SDL_JSTEST_BUILD_TYPE ""
#endif

void report_line(const char* topic, const char* fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fprintf(stderr, "%s: %s\n", topic, buf);
}

static const char* build_type(void)
{
    if (SDL_JSTEST_BUILD_TYPE[0]) return SDL_JSTEST_BUILD_TYPE;
#if defined(NDEBUG)
    return "release";
#else
    return "debug";
#endif
}

static const char* arch(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "aarch64";
#elif defined(__arm__)
    return "arm";
#elif defined(__wasm32__)
    return "wasm32";
#else
    return "unknown arch";
#endif
}

static void compiler(char* buf, size_t size)
{
#if defined(__clang__)
    snprintf(buf, size, "clang %s", __clang_version__);
#elif defined(__GNUC__)
    snprintf(buf, size, "gcc %s", __VERSION__);
#elif defined(_MSC_VER)
    snprintf(buf, size, "msvc %d", _MSC_VER);
#else
    snprintf(buf, size, "unknown compiler");
#endif
}

void report_program(const char* program, const char* version)
{
    char cc[128];
    compiler(cc, sizeof(cc));
    report_line(program, "%s, %s, %s, %s", version, build_type(), arch(), cc);
}

static int cpu_count(void)
{
#if defined(_WIN32)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwNumberOfProcessors;
#elif defined(_SC_NPROCESSORS_ONLN)
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
#else
    return -1;
#endif
}

static int ram_mb(void)
{
#if defined(_WIN32)
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) return -1;
    return (int)(ms.ullTotalPhys / (1024 * 1024));
#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    long pages = sysconf(_SC_PHYS_PAGES), page = sysconf(_SC_PAGESIZE);
    if (pages <= 0 || page <= 0) return -1;
    return (int)((unsigned long long)pages * (unsigned long long)page / (1024 * 1024));
#else
    return -1;
#endif
}

static void os_description(const char* sdl_platform, char* buf, size_t size)
{
#if defined(_WIN32)
    (void)sdl_platform;
    typedef LONG (WINAPI *RtlGetVersionFn)(OSVERSIONINFOW*);
    OSVERSIONINFOW vi;
    memset(&vi, 0, sizeof(vi));
    vi.dwOSVersionInfoSize = sizeof(vi);
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    RtlGetVersionFn fn = ntdll ? (RtlGetVersionFn)(void (*)(void))GetProcAddress(ntdll, "RtlGetVersion") : NULL;
    if (fn && fn(&vi) == 0)
        snprintf(buf, size, "Windows %lu.%lu build %lu",
                 (unsigned long)vi.dwMajorVersion, (unsigned long)vi.dwMinorVersion,
                 (unsigned long)vi.dwBuildNumber);
    else
        snprintf(buf, size, "Windows");
#else
    struct utsname u;
    if (uname(&u) != 0) {
        snprintf(buf, size, "unknown OS");
        return;
    }
#  if defined(__ANDROID__)
    (void)sdl_platform;
    char release[PROP_VALUE_MAX] = "", sdk[PROP_VALUE_MAX] = "";
    char maker[PROP_VALUE_MAX] = "", model[PROP_VALUE_MAX] = "";
    __system_property_get("ro.build.version.release", release);
    __system_property_get("ro.build.version.sdk", sdk);
    __system_property_get("ro.product.manufacturer", maker);
    __system_property_get("ro.product.model", model);
    snprintf(buf, size, "Android %s (API %s), %s %s, Linux %s %s",
             release, sdk, maker, model, u.release, u.machine);
#  else
    /* "Linux, Linux 6.1" says it twice. */
    if (sdl_platform && strcmp(sdl_platform, u.sysname) == 0)
        snprintf(buf, size, "%s %s", u.release, u.machine);
    else
        snprintf(buf, size, "%s %s %s", u.sysname, u.release, u.machine);
#  endif
#endif
}

void report_system(const char* sdl_platform, int cpus, int ram)
{
    char os[256];
    os_description(sdl_platform, os, sizeof(os));
    if (cpus < 0) cpus = cpu_count();
    if (ram < 0) ram = ram_mb();
    if (sdl_platform)
        report_line("system", "%s, %s, %d CPUs, %d MB RAM", sdl_platform, os, cpus, ram);
    else
        report_line("system", "%s, %d CPUs, %d MB RAM", os, cpus, ram);
}

static void env_line(const char* entry)
{
    const char* eq = strchr(entry, '=');
    if (!eq) return;
    size_t value_len = strlen(eq + 1);
    /* Mapping lists can be kilobytes long; the start tells which they are. */
    if (value_len > 100)
        fprintf(stderr, "env: %.*s... (%zu chars)\n", (int)(eq - entry) + 1 + 100, entry, value_len);
    else
        fprintf(stderr, "env: %s\n", entry);
}

void report_env(void)
{
#if defined(_WIN32)
    char** env = _environ;
#else
    char** env = environ;
#endif
    if (!env) return;
    for (char** e = env; *e; ++e) {
        if (strncmp(*e, "SDL_", 4) == 0 || strncmp(*e, "LD_PRELOAD=", 11) == 0 ||
            strncmp(*e, "LD_LIBRARY_PATH=", 16) == 0)
            env_line(*e);
    }
}

void report_ncurses(void)
{
#if defined(SDL_JSTEST_NCURSES)
#  if defined(NCURSES_VERSION)
    report_line("ncurses", "%s (built against %s)", curses_version(), NCURSES_VERSION);
#  else
    report_line("ncurses", "%s", curses_version());
#  endif
#endif
}

const char* report_mapped_library(const char* prefix, char* buf, size_t size)
{
#if defined(__linux__)
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return NULL;
    char line[1024];
    const char* found = NULL;
    while (!found && fgets(line, sizeof(line), fp)) {
        char* path = strchr(line, '/');
        if (!path) continue;
        path[strcspn(path, "\n")] = '\0';
        const char* base = strrchr(path, '/') + 1;
        if (strncmp(base, prefix, strlen(prefix)) == 0) {
            snprintf(buf, size, "%s", path);
            found = buf;
        }
    }
    fclose(fp);
    return found;
#else
    (void)prefix; (void)buf; (void)size;
    return NULL;
#endif
}

/* Start of the bindings: what follows "guid,name,". */
static const char* mapping_bindings(const char* mapping)
{
    const char* p = strchr(mapping, ',');
    if (!p) return NULL;
    p = strchr(p + 1, ',');
    return p ? p + 1 : NULL;
}

static bool text_has_bindings(const char* text, const char* bindings)
{
    if (!text || !bindings) return false;
    char key[41];
    snprintf(key, sizeof(key), "%s", bindings);
    return strstr(text, key) != NULL;
}

static char* read_file(const char* path)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;
    char* data = NULL;
    size_t len = 0, cap = 0;
    for (;;) {
        if (len + 4096 + 1 > cap) {
            cap = cap ? cap * 2 : 65536;
            char* n = realloc(data, cap);
            if (!n) { free(data); fclose(fp); return NULL; }
            data = n;
        }
        size_t got = fread(data + len, 1, 4096, fp);
        len += got;
        if (got == 0) break;
    }
    fclose(fp);
    data[len] = '\0';
    return data;
}

const char* report_mapping_source(const char* mapping, const char* db_text, const char* db_label)
{
    const char* bindings = mapping ? mapping_bindings(mapping) : NULL;
    if (!bindings) return "unknown";
    if (text_has_bindings(getenv("SDL_GAMECONTROLLERCONFIG"), bindings))
        return "SDL_GAMECONTROLLERCONFIG";
    const char* file = getenv("SDL_GAMECONTROLLERCONFIG_FILE");
    if (file && *file) {
        char* text = read_file(file);
        bool hit = text_has_bindings(text, bindings);
        free(text);
        if (hit) return "SDL_GAMECONTROLLERCONFIG_FILE";
    }
    if (text_has_bindings(db_text, bindings))
        return db_label;
    return "SDL's built-in database";
}

const char* platform_exe_dir(void)
{
    static char dir[1024];
#if defined(__ANDROID__)
    return NULL;
#elif defined(__linux__)
    ssize_t n = readlink("/proc/self/exe", dir, sizeof(dir) - 1);
    if (n <= 0) return NULL;
    dir[n] = '\0';
#elif defined(_WIN32)
    DWORD n = GetModuleFileNameA(NULL, dir, (DWORD)sizeof(dir));
    if (n == 0 || n >= sizeof(dir)) return NULL;
    for (char* p = dir; *p; ++p)
        if (*p == '\\') *p = '/';
#else
    return NULL;
#endif
    char* slash = strrchr(dir, '/');
    if (!slash) return NULL;
    slash[1] = '\0';
    return dir;
}

static bool is_dir(const char* path)
{
    struct stat st;
    return stat(path, &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR;
}

void platform_find_data_dir(DataDir* out, const char* option, const char* exe_dir,
                            const char* compiled_in)
{
    out->path[0] = '\0';
    out->source = "not found";
    if (option) {
        snprintf(out->path, sizeof(out->path), "%s", option);
        out->source = "--data-dir";
        return;
    }
#if defined(__ANDROID__)
    (void)exe_dir; (void)compiled_in;
    out->source = "APK assets";
#else
    if (exe_dir) {
#  if defined(_WIN32)
        snprintf(out->path, sizeof(out->path), "%sdata", exe_dir);
#  else
        snprintf(out->path, sizeof(out->path), "%s../share/sdl-jstest", exe_dir);
#  endif
        if (is_dir(out->path)) {
#  if !defined(_WIN32)
            char real[PATH_MAX];
            if (realpath(out->path, real))
                snprintf(out->path, sizeof(out->path), "%s", real);
#  endif
            out->source = "next to the executable";
            return;
        }
        out->path[0] = '\0';
    }
    if (compiled_in && *compiled_in && is_dir(compiled_in)) {
        snprintf(out->path, sizeof(out->path), "%s", compiled_in);
        out->source = "install prefix";
    }
#endif
}

const char* platform_data_file(const DataDir* dir, const char* name, char* buf, size_t size)
{
#if defined(__ANDROID__)
    if (!dir->path[0]) {
        snprintf(buf, size, "%s", name);
        return buf;
    }
#endif
    if (!dir->path[0]) return NULL;
    snprintf(buf, size, "%s/%s", dir->path, name);
    return buf;
}

void platform_setenv(const char* name, const char* value)
{
#if defined(_WIN32)
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

void platform_print_paths(const DataDir* dir)
{
    printf("data: %s (%s)\n", dir->path[0] ? dir->path : "-", dir->source);
}
