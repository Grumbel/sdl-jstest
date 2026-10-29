// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "graph_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ARGB8888 colours — static const, not enum (values exceed signed int range). */
static const uint32_t COL_BG       = 0xFF1A1A2Eu;
static const uint32_t COL_PANEL    = 0xFF16213Eu;
static const uint32_t COL_BORDER   = 0xFF0F3460u;
static const uint32_t COL_TEXT     = 0xFFE8E8E8u;
static const uint32_t COL_DIM      = 0xFF888899u;
static const uint32_t COL_CYAN     = 0xFF4ECDC4u;
static const uint32_t COL_GREEN    = 0xFF2ECC71u;
static const uint32_t COL_YELLOW   = 0xFFF1C40Fu;
static const uint32_t COL_BAR_BG   = 0xFF2A2A3Eu;
static const uint32_t COL_BAR_FILL = 0xFF3498DBu;
static const uint32_t COL_BAR_MID  = 0xFF7F8C8Du;
static const uint32_t COL_BTN_OFF  = 0xFF3D3D5Cu;
static const uint32_t COL_BTN_ON   = 0xFF27AE60u;
static const uint32_t COL_SEL_BG   = 0xFF4A90D9u;
static const uint32_t COL_HAT_ON   = 0xFFF39C12u;
static const uint32_t COL_HAT_OFF  = 0xFF555566u;

static const uint8_t font8x8_basic[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
    {0x62,0x64,0x08,0x10,0x20,0x4C,0x8C,0x00},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x0C},
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00},
    {0x40,0x20,0x10,0x08,0x04,0x02,0x01,0x00},
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},
    {0x18,0x1C,0x18,0x18,0x18,0x18,0x7E,0x00},
    {0x3C,0x66,0x60,0x30,0x18,0x0C,0x7E,0x00},
    {0x3C,0x66,0x60,0x38,0x60,0x66,0x3C,0x00},
    {0x30,0x38,0x34,0x32,0x7E,0x30,0x30,0x00},
    {0x7E,0x06,0x3E,0x60,0x60,0x66,0x3C,0x00},
    {0x38,0x0C,0x06,0x3E,0x66,0x66,0x3C,0x00},
    {0x7E,0x60,0x30,0x18,0x0C,0x0C,0x0C,0x00},
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
    {0x3C,0x66,0x66,0x7C,0x60,0x30,0x1C,0x00},
    {0x00,0x00,0x18,0x00,0x00,0x18,0x00,0x00},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
    {0x00,0x30,0x18,0x0C,0x18,0x30,0x00,0x00},
    {0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00},
    {0x00,0x0C,0x18,0x30,0x18,0x0C,0x00,0x00},
    {0x3C,0x66,0x60,0x30,0x18,0x00,0x18,0x00},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},
    {0x3E,0x66,0x66,0x3E,0x66,0x66,0x3E,0x00},
    {0x3C,0x66,0x06,0x06,0x06,0x66,0x3C,0x00},
    {0x1E,0x36,0x66,0x66,0x66,0x36,0x1E,0x00},
    {0x7E,0x06,0x06,0x3E,0x06,0x06,0x7E,0x00},
    {0x7E,0x06,0x06,0x3E,0x06,0x06,0x06,0x00},
    {0x3C,0x66,0x06,0x76,0x66,0x66,0x3C,0x00},
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    {0x78,0x30,0x30,0x30,0x30,0x36,0x1C,0x00},
    {0x66,0x36,0x1E,0x0E,0x1E,0x36,0x66,0x00},
    {0x06,0x06,0x06,0x06,0x06,0x06,0x7E,0x00},
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
    {0x66,0x6E,0x7E,0x7E,0x76,0x66,0x66,0x00},
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    {0x3E,0x66,0x66,0x3E,0x06,0x06,0x06,0x00},
    {0x3C,0x66,0x66,0x66,0x76,0x3C,0x60,0x00},
    {0x3E,0x66,0x66,0x3E,0x1E,0x36,0x66,0x00},
    {0x3C,0x66,0x06,0x3C,0x60,0x66,0x3C,0x00},
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
    {0x7E,0x60,0x30,0x18,0x0C,0x06,0x7E,0x00},
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
    {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x00},
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},
    {0x3E,0x66,0x66,0x3E,0x66,0x66,0x3E,0x00},
    {0x3C,0x66,0x06,0x06,0x06,0x66,0x3C,0x00},
    {0x1E,0x36,0x66,0x66,0x66,0x36,0x1E,0x00},
    {0x7E,0x06,0x06,0x3E,0x06,0x06,0x7E,0x00},
    {0x7E,0x06,0x06,0x3E,0x06,0x06,0x06,0x00},
    {0x3C,0x66,0x06,0x76,0x66,0x66,0x3C,0x00},
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    {0x78,0x30,0x30,0x30,0x30,0x36,0x1C,0x00},
    {0x66,0x36,0x1E,0x0E,0x1E,0x36,0x66,0x00},
    {0x06,0x06,0x06,0x06,0x06,0x06,0x7E,0x00},
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
    {0x66,0x6E,0x7E,0x7E,0x76,0x66,0x66,0x00},
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    {0x3E,0x66,0x66,0x3E,0x06,0x06,0x06,0x00},
    {0x3C,0x66,0x66,0x66,0x76,0x3C,0x60,0x00},
    {0x3E,0x66,0x66,0x3E,0x1E,0x36,0x66,0x00},
    {0x3C,0x66,0x06,0x3C,0x60,0x66,0x3C,0x00},
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
    {0x7E,0x60,0x30,0x18,0x0C,0x06,0x7E,0x00},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
    {0x7E,0x42,0x42,0x42,0x42,0x42,0x7E,0x00},
};


static void put_pixel(GraphUI* ui, int x, int y, uint32_t c)
{
    if ((unsigned)x < (unsigned)ui->w && (unsigned)y < (unsigned)ui->h)
        ui->pixels[y * ui->w + x] = c;
}

static void fill_rect(GraphUI* ui, int x, int y, int w, int h, uint32_t c)
{
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > ui->w) w = ui->w - x;
    if (y + h > ui->h) h = ui->h - y;
    if (w <= 0 || h <= 0) return;
    for (int row = 0; row < h; ++row) {
        uint32_t* p = ui->pixels + (y + row) * ui->w + x;
        for (int col = 0; col < w; ++col) p[col] = c;
    }
}

static void draw_rect_outline(GraphUI* ui, int x, int y, int w, int h, uint32_t c)
{
    fill_rect(ui, x, y, w, 1, c);
    fill_rect(ui, x, y + h - 1, w, 1, c);
    fill_rect(ui, x, y, 1, h, c);
    fill_rect(ui, x + w - 1, y, 1, h, c);
}

static void draw_char(GraphUI* ui, int x, int y, char ch, uint32_t c)
{
    unsigned idx = (ch < 32 || ch > 126) ? 0u : (unsigned)(ch - 32);
    const uint8_t* g = font8x8_basic[idx];
    for (int row = 0; row < 8; ++row) {
        uint8_t bits = g[row];
        /* Font bytes are LSB-left (bit 0 = leftmost pixel). */
        for (int col = 0; col < 8; ++col)
            if (bits & (1u << col))
                put_pixel(ui, x + col, y + row, c);
    }
}

static void draw_text(GraphUI* ui, int x, int y, const char* s, uint32_t c)
{
    if (!s) return;
    while (*s) {
        draw_char(ui, x, y, *s, c);
        x += 8;
        if (x >= ui->w) break;
        s++;
    }
}

static int text_width(const char* s) { return s ? (int)strlen(s) * 8 : 0; }

bool graph_ui_init(GraphUI* ui, int width, int height)
{
    memset(ui, 0, sizeof(*ui));
    if (width < 320) width = 320;
    if (height < 240) height = 240;
    ui->w = width; ui->h = height;
    ui->pixels = calloc((size_t)width * (size_t)height, sizeof(uint32_t));
    if (!ui->pixels) return false;
    ui->dirty = true;
    ui->menu_numbered = -1;
    graph_ui_log(ui, "Event log ready — device activity appears here.");
    return true;
}

void graph_ui_shutdown(GraphUI* ui)
{
    free(ui->pixels);
    ui->pixels = NULL;
    ui->w = ui->h = 0;
}

void graph_ui_log(GraphUI* ui, const char* fmt, ...)
{
    char line[GRAPH_UI_LOG_LEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    snprintf(ui->log[ui->log_head], GRAPH_UI_LOG_LEN, "%s", line);
    ui->log_head = (ui->log_head + 1) % GRAPH_UI_LOG_CAP;
    if (ui->log_len < GRAPH_UI_LOG_CAP) ui->log_len++;
    ui->dirty = true;
}

void graph_ui_clear(GraphUI* ui, uint32_t argb)
{
    fill_rect(ui, 0, 0, ui->w, ui->h, argb);
}

static void draw_axis_bar(GraphUI* ui, int x, int y, int bar_w, int bar_h, int16_t value)
{
    fill_rect(ui, x, y, bar_w, bar_h, COL_BAR_BG);
    draw_rect_outline(ui, x, y, bar_w, bar_h, COL_BORDER);
    int mid = bar_w / 2;
    int pos = (int)(((int)value + 32768) * (bar_w - 1) / 65535);
    if (pos < 0) pos = 0;
    if (pos >= bar_w) pos = bar_w - 1;
    if (value < 0) {
        int width = mid - pos;
        if (width > 0) fill_rect(ui, x + pos, y + 1, width, bar_h - 2, COL_BAR_FILL);
    } else if (value > 0) {
        int width = pos - mid;
        if (width > 0) fill_rect(ui, x + mid, y + 1, width, bar_h - 2, COL_BAR_FILL);
    }
    fill_rect(ui, x + mid, y, 1, bar_h, COL_BAR_MID);
    fill_rect(ui, x + pos - 1, y, 3, bar_h, COL_YELLOW);
}

static void draw_hat(GraphUI* ui, int x, int y, uint8_t hat)
{
    const int cell = 14, gap = 2;
    int cx = x + cell + gap, cy = y + cell + gap;
    fill_rect(ui, cx, y, cell, cell, (hat & 0x01) ? COL_HAT_ON : COL_HAT_OFF);
    fill_rect(ui, x, cy, cell, cell, (hat & 0x08) ? COL_HAT_ON : COL_HAT_OFF);
    fill_rect(ui, cx, cy, cell, cell, hat ? COL_CYAN : COL_HAT_OFF);
    fill_rect(ui, cx + cell + gap, cy, cell, cell, (hat & 0x02) ? COL_HAT_ON : COL_HAT_OFF);
    fill_rect(ui, cx, cy + cell + gap, cell, cell, (hat & 0x04) ? COL_HAT_ON : COL_HAT_OFF);
}

static void draw_event_log(GraphUI* ui, int x, int y, int w, int h)
{
    fill_rect(ui, x, y, w, h, COL_PANEL);
    draw_rect_outline(ui, x, y, w, h, COL_BORDER);
    draw_text(ui, x + 6, y + 4, "Events", COL_CYAN);
    int line_h = 11, top = y + 18;
    int visible = (h - 24) / line_h;
    if (visible < 1) return;
    int start = ui->log_len > visible ? ui->log_len - visible : 0;
    for (int i = 0; i < visible && start + i < ui->log_len; ++i) {
        int logical = start + i;
        int idx = (ui->log_len < GRAPH_UI_LOG_CAP) ? logical
                  : (ui->log_head + logical) % GRAPH_UI_LOG_CAP;
        char line[GRAPH_UI_LOG_LEN];
        snprintf(line, sizeof(line), "%s", ui->log[idx]);
        int max_chars = (w - 16) / 8;
        if (max_chars > 0 && (int)strlen(line) > max_chars) line[max_chars] = '\0';
        draw_text(ui, x + 8, top + i * line_h, line, COL_TEXT);
    }
}

void graph_ui_draw_joystick(GraphUI* ui, const char* name, int joy_idx,
    const int16_t* axes, int num_axes, const uint8_t* buttons, int num_buttons,
    const char* const* button_labels, const TestUIButtonGroup* button_groups, int num_groups,
    const uint8_t* hats, int num_hats, const int16_t* balls, int num_balls)
{
    graph_ui_clear(ui, COL_BG);
    /* Compact margins so 640x480 still fits axes + log. */
    int margin = (ui->w < 800 || ui->h < 600) ? 6 : 12;
    int pad = (ui->w < 800 || ui->h < 600) ? 8 : 12;
    int events_min = (ui->h < 600) ? 72 : 80;
    int status_h = ui->h * 2 / 3;
    if (status_h < 160)
        status_h = ui->h - events_min - margin;
    int events_h = ui->h - status_h - margin;
    if (events_h < events_min) {
        events_h = events_min;
        status_h = ui->h - events_h - margin;
    }
    if (status_h < 80)
        status_h = ui->h / 2;

    fill_rect(ui, margin, margin, ui->w - 2 * margin, status_h - margin, COL_PANEL);
    draw_rect_outline(ui, margin, margin, ui->w - 2 * margin, status_h - margin, COL_BORDER);

    int x = margin + pad, y = margin + pad;
    int max_x = ui->w - margin - pad, max_y = status_h - 6;
    int label_w = 80; /* space for "nn +32767 " before axis bar */

    /* "Menu" button for touch and mouse, top right of the status panel. */
    ui->back_w = text_width("Menu") + 16;
    ui->back_h = 20;
    ui->back_x = ui->w - margin - pad - ui->back_w;
    ui->back_y = margin + pad - 4;
    fill_rect(ui, ui->back_x, ui->back_y, ui->back_w, ui->back_h, COL_BTN_OFF);
    draw_rect_outline(ui, ui->back_x, ui->back_y, ui->back_w, ui->back_h, COL_BORDER);
    draw_text(ui, ui->back_x + 8, ui->back_y + 6, "Menu", COL_TEXT);

    {
        char title[128];
        snprintf(title, sizeof(title), "%s", name ? name : "(unknown)");
        int max_chars = (ui->back_x - 8 - x) / 8;
        if (max_chars > 0 && (int)strlen(title) > max_chars) title[max_chars] = '\0';
        draw_text(ui, x, y, title, COL_CYAN);
    }
    y += 12;
    char meta[128];
    snprintf(meta, sizeof(meta), "Index %d  Axes %d  Buttons %d  Hats %d  Balls %d",
             joy_idx, num_axes, num_buttons, num_hats, num_balls);
    draw_text(ui, x, y, meta, COL_DIM);
    y += 14;

    if (num_axes > 0 && y < max_y) {
        draw_text(ui, x, y, "Axes", COL_YELLOW);
        y += 12;
        int bar_w = max_x - x - label_w;
        if (bar_w < 48) bar_w = 48;
        int bar_h = (ui->h < 600) ? 10 : 12;
        int row_gap = (ui->h < 600) ? 14 : 18;
        for (int i = 0; i < num_axes && y + bar_h < max_y; ++i) {
            char lab[32];
            snprintf(lab, sizeof(lab), "%2d %+6d", i, (int)axes[i]);
            draw_text(ui, x, y + 1, lab, COL_TEXT);
            draw_axis_bar(ui, x + label_w, y, bar_w, bar_h, axes[i]);
            y += row_gap;
        }
        y += 4;
    }

    if (num_buttons > 0 && y < max_y) {
        draw_text(ui, x, y, "Buttons", COL_GREEN);
        y += 14;
        TestUIButtonGroup fallback = { 0, num_buttons };
        const TestUIButtonGroup* groups = button_groups;
        int ng = num_groups;
        if (!groups || ng <= 0) { groups = &fallback; ng = 1; }
        for (int g = 0; g < ng && y < max_y; ++g) {
            int begin = groups[g].begin, end_i = groups[g].end;
            if (begin < 0) begin = 0;
            if (end_i > num_buttons) end_i = num_buttons;
            if (begin >= end_i) continue;
            int cx = x, cy = y, row_h = 28;
            for (int i = begin; i < end_i; ++i) {
                const char* lab = (button_labels && button_labels[i] && button_labels[i][0])
                    ? button_labels[i] : NULL;
                char buf[32];
                if (!lab) { snprintf(buf, sizeof(buf), "%d", i); lab = buf; }
                int bw = text_width(lab) + 16;
                if (bw < 28) bw = 28;
                if (cx + bw > max_x) { cx = x; cy += row_h; if (cy + row_h > max_y) break; }
                fill_rect(ui, cx, cy, bw, 22, buttons[i] ? COL_BTN_ON : COL_BTN_OFF);
                draw_rect_outline(ui, cx, cy, bw, 22, COL_BORDER);
                draw_text(ui, cx + 8, cy + 7, lab, COL_TEXT);
                cx += bw + 6;
            }
            y = cy + row_h + 4;
        }
        y += 4;
    }

    if (num_hats > 0 && y < max_y) {
        draw_text(ui, x, y, "Hats", COL_YELLOW);
        y += 14;
        int hx = x;
        for (int i = 0; i < num_hats && y + 50 < max_y; ++i) {
            char lab[16];
            snprintf(lab, sizeof(lab), "%d", i);
            draw_text(ui, hx, y, lab, COL_DIM);
            draw_hat(ui, hx + 16, y, hats[i]);
            hx += 70;
            if (hx + 60 > max_x) { hx = x; y += 56; }
        }
        y += 56;
    }

    if (num_balls > 0 && y < max_y) {
        draw_text(ui, x, y, "Trackballs", COL_YELLOW);
        y += 14;
        for (int i = 0; i < num_balls && y < max_y; ++i) {
            char lab[64];
            snprintf(lab, sizeof(lab), "%2d  dx %+6d  dy %+6d",
                     i, (int)balls[2 * i], (int)balls[2 * i + 1]);
            draw_text(ui, x, y, lab, COL_TEXT);
            y += 12;
        }
    }

    if (status_h - 14 > y) {
        const char* hint = ui->hint ? ui->hint : "Esc/q: menu   Ctrl-C or close window: exit app";
        draw_text(ui, x, status_h - 14, hint, COL_DIM);
    }
    if (ui->hold_progress > 0.0f) {
        int bw = max_x - x;
        fill_rect(ui, x, status_h - 4, bw, 3, COL_BAR_BG);
        fill_rect(ui, x, status_h - 4, (int)((float)bw * ui->hold_progress), 3, COL_YELLOW);
    }
    draw_event_log(ui, margin, status_h, ui->w - 2 * margin, events_h);
    ui->dirty = false;
}

void graph_ui_draw_menu(GraphUI* ui, const char* title,
                        const char* const* items, int n_items)
{
    graph_ui_clear(ui, COL_BG);
    int margin = (ui->w < 800 || ui->h < 600) ? 6 : 12;
    int pad = (ui->w < 800 || ui->h < 600) ? 10 : 16;
    fill_rect(ui, margin, margin, ui->w - 2 * margin, ui->h - 2 * margin, COL_PANEL);
    draw_rect_outline(ui, margin, margin, ui->w - 2 * margin, ui->h - 2 * margin, COL_BORDER);
    int x = margin + pad, y = margin + pad;
    draw_text(ui, x, y, title ? title : "Select device", COL_CYAN);
    y += 14;
    draw_text(ui, x, y, ui->hint ? ui->hint : "Up/Down  Enter  Esc/q quit", COL_DIM);
    y += 18;
    ui->menu_items = n_items > 0 ? n_items : 0;
    ui->menu_visible = 0;
    if (n_items <= 0) {
        draw_text(ui, x, y, "(no devices detected)", COL_TEXT);
    } else {
        int line_h = 18;
        int visible = (ui->h - y - 40) / line_h;
        if (visible < 1) visible = 1;
        if (ui->menu_selected >= n_items) ui->menu_selected = n_items - 1;
        if (ui->menu_selected < 0) ui->menu_selected = 0;
        int start = 0;
        if (ui->menu_selected >= start + visible)
            start = ui->menu_selected - visible + 1;
        if (ui->menu_selected < start) start = ui->menu_selected;
        ui->menu_x = x - 4;
        ui->menu_y = y - 2;
        ui->menu_w = ui->w - 2 * margin - 24;
        ui->menu_line_h = line_h;
        ui->menu_start = start;
        ui->menu_visible = visible;
        for (int i = 0; i < visible && start + i < n_items; ++i) {
            int idx = start + i, row = y + i * line_h;
            if (idx == ui->menu_selected)
                fill_rect(ui, x - 4, row - 2, ui->w - 2 * margin - 24, line_h, COL_SEL_BG);
            char line[256];
            if (ui->menu_numbered < 0 || idx < ui->menu_numbered)
                snprintf(line, sizeof(line), "%s %2d  %s",
                         idx == ui->menu_selected ? ">" : " ", idx,
                         items[idx] ? items[idx] : "?");
            else
                snprintf(line, sizeof(line), "%s     %s",
                         idx == ui->menu_selected ? ">" : " ",
                         items[idx] ? items[idx] : "?");
            int max_chars = (ui->w - 2 * margin - 40) / 8;
            if (max_chars > 0 && (int)strlen(line) > max_chars) line[max_chars] = '\0';
            draw_text(ui, x, row, line, COL_TEXT);
        }
    }
    draw_text(ui, x, ui->h - margin - 16, "sdl-jstest", COL_DIM);
    ui->dirty = false;
}

int graph_ui_menu_key(GraphUI* ui, int key, int n_items)
{
    if (key == 27 || key == 'q' || key == 'Q') return -1;
    if (key == '\n' || key == '\r' || key == ' ')
        return (n_items > 0) ? ui->menu_selected : -2;
    if (key == GRAPH_UI_KEY_UP || key == 'k' || key == 'K') {
        if (n_items > 0)
            ui->menu_selected = (ui->menu_selected + n_items - 1) % n_items;
        return -2;
    }
    if (key == GRAPH_UI_KEY_DOWN || key == 'j' || key == 'J') {
        if (n_items > 0)
            ui->menu_selected = (ui->menu_selected + 1) % n_items;
        return -2;
    }
    if (key >= '0' && key <= '9' && n_items > 0) {
        int d = key - '0';
        if (d < n_items) ui->menu_selected = d;
        return -2;
    }
    return -2;
}

int graph_ui_menu_hit(const GraphUI* ui, int x, int y)
{
    if (ui->menu_visible <= 0 || ui->menu_line_h <= 0) return -1;
    if (x < ui->menu_x || x >= ui->menu_x + ui->menu_w || y < ui->menu_y) return -1;
    int row = (y - ui->menu_y) / ui->menu_line_h;
    if (row >= ui->menu_visible) return -1;
    int idx = ui->menu_start + row;
    return idx < ui->menu_items ? idx : -1;
}

bool graph_ui_back_hit(const GraphUI* ui, int x, int y)
{
    return ui->back_w > 0 && x >= ui->back_x && x < ui->back_x + ui->back_w &&
           y >= ui->back_y && y < ui->back_y + ui->back_h;
}
