// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "test_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GRAPH_UI_LOG_CAP 128
#define GRAPH_UI_LOG_LEN 160
#define GRAPH_UI_DEFAULT_W 640
#define GRAPH_UI_DEFAULT_H 480
#define GRAPH_UI_KEY_UP 0x100
#define GRAPH_UI_KEY_DOWN 0x101

typedef struct GraphUI {
    uint32_t* pixels;
    int w, h;
    char log[GRAPH_UI_LOG_CAP][GRAPH_UI_LOG_LEN];
    int log_len, log_head;
    bool dirty;
    int menu_selected;
    /* Set by the caller before drawing: the key help line, and how far a
       hold-to-leave is (0..1, drawn as a bar when > 0). */
    const char* hint;
    float hold_progress;
    /* Menu rows from this index on are commands ("Quit") without a device
       number; -1: all rows are numbered. */
    int menu_numbered;
    /* Written while drawing, for pointer hit tests. */
    int menu_x, menu_y, menu_w, menu_line_h, menu_start, menu_visible, menu_items;
    int back_x, back_y, back_w, back_h;
} GraphUI;

bool graph_ui_init(GraphUI* ui, int width, int height);
void graph_ui_shutdown(GraphUI* ui);
void graph_ui_log(GraphUI* ui, const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;
void graph_ui_clear(GraphUI* ui, uint32_t argb);
void graph_ui_draw_joystick(GraphUI* ui, const char* name, int joy_idx,
    const int16_t* axes, int num_axes, const uint8_t* buttons, int num_buttons,
    const char* const* button_labels, const TestUIButtonGroup* button_groups, int num_groups,
    const uint8_t* hats, int num_hats, const int16_t* balls, int num_balls);
void graph_ui_draw_menu(GraphUI* ui, const char* title, const char* const* items, int n_items);
int graph_ui_menu_key(GraphUI* ui, int key, int n_items);
/* Menu row under (x, y) in buffer pixels from the last draw, or -1. */
int graph_ui_menu_hit(const GraphUI* ui, int x, int y);
/* Whether (x, y) is on the device view's "Menu" button. */
bool graph_ui_back_hit(const GraphUI* ui, int x, int y);

#ifdef __cplusplus
}
#endif
