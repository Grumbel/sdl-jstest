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
    void* status_win;
    void* events_win;
    int status_h;
    int events_h;
    char log[TEST_UI_LOG_CAP][TEST_UI_LOG_LEN];
    int log_len;
    int log_head;
    bool dirty;
} TestUI;

typedef struct TestUIButtonGroup {
    int begin;
    int end;
} TestUIButtonGroup;

/** Result of polling keyboard while a screen is active. */
typedef enum TestUIAction {
    TEST_UI_CONTINUE = 0,
    TEST_UI_BACK,   /* Esc: leave current screen (to menu or exit menu) */
    TEST_UI_QUIT    /* Ctrl-C: leave the whole program */
} TestUIAction;

void test_ui_init(TestUI* ui);
void test_ui_shutdown(TestUI* ui);
void test_ui_relayout(TestUI* ui);

void test_ui_log(TestUI* ui, const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

void test_ui_draw_joystick(TestUI* ui,
                           const char* name,
                           int joy_idx,
                           const int16_t* axes, int num_axes,
                           const uint8_t* buttons, int num_buttons,
                           const char* const* button_labels,
                           const TestUIButtonGroup* button_groups, int num_groups,
                           const uint8_t* hats, int num_hats,
                           const int16_t* balls, int num_balls);

void test_ui_draw_events(TestUI* ui);
void test_ui_refresh(TestUI* ui);

/** Drain input; returns BACK (Esc), QUIT (Ctrl-C), or CONTINUE. */
TestUIAction test_ui_poll_action(TestUI* ui);

/**
 * Interactive device picker. Arrow keys / j/k move, Enter selects, Esc cancels.
 * @param items device labels
 * @param n_items count
 * @return selected index, or -1 if cancelled (Esc / quit)
 */
int test_ui_menu_select(TestUI* ui, const char* title,
                        const char* const* items, int n_items);

#ifdef __cplusplus
}
#endif
