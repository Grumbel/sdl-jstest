// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/* Offline tests of the SDL-independent code: option parsing, menu
   navigation helpers, mapping sources. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/cli.h"
#include "common/report.h"
#include "ui/gui_nav.h"

static int g_failed = 0;
static int g_passed = 0;

#define CHECK(cond) do { \
    if (cond) { ++g_passed; } \
    else { ++g_failed; fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

static unsigned const kAll = JSTEST_CLI_GAMECONTROLLER | JSTEST_CLI_RUMBLE;

static int parse(JstestOptions* opts, unsigned features, int argc, char** argv)
{
    return cli_parse(argc, argv, features, opts) ? 1 : 0;
}

static void test_cli(void)
{
    JstestOptions o;
    {
        char* argv[] = { "x" };
        CHECK(parse(&o, kAll, 1, argv));
        CHECK(o.mode == JSTEST_MODE_DEFAULT && o.joy_idx == -1 && o.fullscreen == -1);
    }
    {
        char* argv[] = { "x", "--test", "3" };
        CHECK(parse(&o, kAll, 3, argv));
        CHECK(o.mode == JSTEST_MODE_TEST && o.joy_idx == 3);
    }
    {
        char* argv[] = { "x", "-t2" };
        CHECK(parse(&o, kAll, 2, argv));
        CHECK(o.mode == JSTEST_MODE_TEST && o.joy_idx == 2);
    }
    {
        char* argv[] = { "x", "--rumble=1" };
        CHECK(parse(&o, kAll, 2, argv));
        CHECK(o.mode == JSTEST_MODE_RUMBLE && o.joy_idx == 1);
    }
    {
        /* --gui takes JOYNUM only when it is a number. */
        char* argv[] = { "x", "--gui", "--fullscreen" };
        CHECK(parse(&o, kAll, 3, argv));
        CHECK(o.mode == JSTEST_MODE_GUI && o.joy_idx == -1 && o.fullscreen == 1);
    }
    {
        char* argv[] = { "x", "-G", "0", "-w", "--geometry", "800x600" };
        CHECK(parse(&o, kAll, 6, argv));
        CHECK(o.mode == JSTEST_MODE_GUI && o.joy_idx == 0 && o.fullscreen == 0);
        CHECK(o.width == 800 && o.height == 600);
    }
    {
        char* argv[] = { "x", "--data-dir=/opt/x", "-l" };
        CHECK(parse(&o, kAll, 3, argv));
        CHECK(o.mode == JSTEST_MODE_LIST && o.data_dir && strcmp(o.data_dir, "/opt/x") == 0);
    }
    /* Errors (OPT-1): unknown options, bad values, two modes, stray args. */
    {
        char* argv[] = { "x", "--bogus" };
        CHECK(!parse(&o, kAll, 2, argv));
    }
    {
        char* argv[] = { "x", "--test", "abc" };
        CHECK(!parse(&o, kAll, 3, argv));
    }
    {
        char* argv[] = { "x", "--test" };
        CHECK(!parse(&o, kAll, 2, argv));
    }
    {
        char* argv[] = { "x", "--list", "--test", "1" };
        CHECK(!parse(&o, kAll, 4, argv));
    }
    {
        char* argv[] = { "x", "--geometry", "800" };
        CHECK(!parse(&o, kAll, 3, argv));
    }
    {
        char* argv[] = { "x", "--", "--list" };
        CHECK(!parse(&o, kAll, 3, argv));
    }
    {
        char* argv[] = { "x", "--help=1" };
        CHECK(!parse(&o, kAll, 2, argv));
    }
    {
        /* sdl1-jstest has no gamecontroller or rumble mode. */
        char* argv[] = { "x", "-g", "0" };
        CHECK(!parse(&o, 0, 3, argv));
        CHECK(parse(&o, kAll, 3, argv));
    }
}

static void test_stick(void)
{
    GuiStick s = { 0 };
    CHECK(gui_stick_axis(&s, 0) == GUI_NONE);
    CHECK(gui_stick_axis(&s, 30000) == GUI_DOWN);
    CHECK(gui_stick_axis(&s, 32000) == GUI_NONE);
    CHECK(gui_stick_axis(&s, -30000) == GUI_NONE);   /* centre first */
    CHECK(gui_stick_axis(&s, 100) == GUI_NONE);
    CHECK(gui_stick_axis(&s, -30000) == GUI_UP);
}

static void test_hold(void)
{
    GuiHold h = { 0 };
    CHECK(gui_hold_progress(&h, 100, 1000) == 0.0f);
    gui_hold_set(&h, true, 100);
    CHECK(gui_hold_progress(&h, 600, 1000) == 0.5f);
    gui_hold_set(&h, true, 900);   /* still down: keeps the start */
    CHECK(gui_hold_progress(&h, 1100, 1000) == 1.0f);
    gui_hold_set(&h, false, 1200);
    CHECK(gui_hold_progress(&h, 1300, 1000) == 0.0f);
}

static void test_mapping_source(void)
{
    const char* db = "030000005e0400008e02000014010000,X360 Controller,a:b0,b:b1,x:b2,y:b3,back:b6,guide:b8,start:b7,platform:Linux,\n";
    /* SDL rewrites the GUID (CRC) and may rename; the bindings decide. */
    CHECK(strcmp(report_mapping_source("03001234aaaa,Other name,a:b0,b:b1,x:b2,y:b3,back:b6,guide:b8,start:b7,platform:Linux",
                                       db, "db"), "db") == 0);
    CHECK(strcmp(report_mapping_source("0300,X,a:b1,b:b0", db, "db"), "SDL's built-in database") == 0);
    CHECK(strcmp(report_mapping_source("garbage", db, "db"), "unknown") == 0);
}

int main(void)
{
    test_cli();
    test_stick();
    test_hold();
    test_mapping_source();
    printf("%d passed, %d failed\n", g_passed, g_failed);
    return g_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
