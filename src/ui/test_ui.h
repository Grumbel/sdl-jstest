// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TEST_UI_LOG_CAP  128
#define TEST_UI_LOG_LEN  160

typedef struct TestUI {
    void* status_win; /* WINDOW* */
    void* events_win; /* WINDOW* */
    int status_h;
    int events_h;
    char log[TEST_UI_LOG_CAP][TEST_UI_LOG_LEN];
    int log_len;   /* number of valid entries (max TEST_UI_LOG_CAP) */
    int log_head;  /* next write index */
    bool dirty;
} TestUI;

/** Initialize locale, ncurses, and split panes. */
void test_ui_init(TestUI* ui);

/** Tear down ncurses. */
void test_ui_shutdown(TestUI* ui);

/** Recreate panes after terminal resize. */
void test_ui_relayout(TestUI* ui);

/** Append a line to the bottom event log (printf-style). */
void test_ui_log(TestUI* ui, const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

/**
 * Draw the top status pane: identity, axes, wrapping buttons, hats, balls.
 * @param button_labels optional; if non-NULL, length num_buttons (may be NULL entries)
 */
void test_ui_draw_joystick(TestUI* ui,
                           const char* name,
                           int joy_idx,
                           const int16_t* axes, int num_axes,
                           const uint8_t* buttons, int num_buttons,
                           const char* const* button_labels,
                           const uint8_t* hats, int num_hats,
                           const int16_t* balls, int num_balls);

/** Redraw event log pane from the ring buffer. */
void test_ui_draw_events(TestUI* ui);

/** Refresh both panes. */
void test_ui_refresh(TestUI* ui);

/** Non-blocking: true if Ctrl-C was pressed. */
bool test_ui_check_quit(TestUI* ui);

#ifdef __cplusplus
}
#endif
