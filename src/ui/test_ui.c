// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "test_ui.h"

#include <curses.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static WINDOW* as_win(void* p)
{
    return (WINDOW*)p;
}

/* Approximate terminal columns for a UTF-8 string (●/○/─ count as 1). */
static int utf8_cols(const char* s)
{
    int cols = 0;
    const unsigned char* p = (const unsigned char*)s;
    while (*p)
    {
        if ((*p & 0x80) == 0)
        {
            cols++;
            p++;
        }
        else if ((*p & 0xE0) == 0xC0)
        {
            cols++;
            p += 2;
        }
        else if ((*p & 0xF0) == 0xE0)
        {
            cols++;
            p += 3;
        }
        else if ((*p & 0xF8) == 0xF0)
        {
            cols++;
            p += 4;
        }
        else
        {
            p++;
        }
    }
    return cols;
}

void test_ui_init(TestUI* ui)
{
    memset(ui, 0, sizeof(*ui));
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0);
    keypad(stdscr, TRUE);
    if (has_colors())
    {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_CYAN, -1);
        init_pair(2, COLOR_GREEN, -1);
        init_pair(3, COLOR_YELLOW, -1);
        init_pair(4, COLOR_BLACK, COLOR_WHITE);
        init_pair(5, COLOR_RED, -1);
    }
    test_ui_relayout(ui);
    test_ui_log(ui, "Event log ready — device activity appears here.");
}

void test_ui_shutdown(TestUI* ui)
{
    if (ui->status_win)
        delwin(as_win(ui->status_win));
    if (ui->events_win)
        delwin(as_win(ui->events_win));
    ui->status_win = NULL;
    ui->events_win = NULL;
    endwin();
}

void test_ui_relayout(TestUI* ui)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (ui->status_win)
        delwin(as_win(ui->status_win));
    if (ui->events_win)
        delwin(as_win(ui->events_win));

    int status_h = rows * 2 / 3;
    if (status_h < 8)
        status_h = rows > 10 ? rows - 5 : (rows > 1 ? rows / 2 : 1);
    if (status_h >= rows)
        status_h = rows > 1 ? rows - 1 : 1;
    int events_h = rows - status_h;
    if (events_h < 3 && rows > 3)
    {
        events_h = 3;
        status_h = rows - events_h;
    }

    ui->status_h = status_h;
    ui->events_h = events_h;
    ui->status_win = newwin(status_h, cols, 0, 0);
    ui->events_win = newwin(events_h, cols, status_h, 0);
    scrollok(as_win(ui->events_win), TRUE);
    ui->dirty = true;
}

void test_ui_log(TestUI* ui, const char* fmt, ...)
{
    char line[TEST_UI_LOG_LEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);

    snprintf(ui->log[ui->log_head], TEST_UI_LOG_LEN, "%s", line);
    ui->log_head = (ui->log_head + 1) % TEST_UI_LOG_CAP;
    if (ui->log_len < TEST_UI_LOG_CAP)
        ui->log_len++;
    ui->dirty = true;
}

static void draw_axis_bar(WINDOW* w, int16_t value, int bar_cols)
{
    if (bar_cols < 4)
        bar_cols = 4;

    int mid = bar_cols / 2;
    int pos = (int)(((int)value + 32768) * (bar_cols - 1) / 65535);
    if (pos < 0)
        pos = 0;
    if (pos >= bar_cols)
        pos = bar_cols - 1;

    waddstr(w, "│");
    for (int i = 0; i < bar_cols; ++i)
    {
        if (i == mid)
        {
            waddstr(w, (i == pos) ? "◆" : "┼");
        }
        else if (value < 0 && i >= pos && i < mid)
        {
            waddstr(w, "█");
        }
        else if (value > 0 && i > mid && i <= pos)
        {
            waddstr(w, "█");
        }
        else if (i == pos)
        {
            waddstr(w, "●");
        }
        else
        {
            waddstr(w, "─");
        }
    }
    waddstr(w, "│");
}

static void draw_hat(WINDOW* w, uint8_t hat)
{
    int up = hat & 0x01;
    int right = hat & 0x02;
    int down = hat & 0x04;
    int left = hat & 0x08;

    wprintw(w, " U%s D%s L%s R%s  ",
            up ? "▲" : "·",
            down ? "▼" : "·",
            left ? "◀" : "·",
            right ? "▶" : "·");
}

void test_ui_draw_joystick(TestUI* ui,
                           const char* name,
                           int joy_idx,
                           const int16_t* axes, int num_axes,
                           const uint8_t* buttons, int num_buttons,
                           const char* const* button_labels,
                           const uint8_t* hats, int num_hats,
                           const int16_t* balls, int num_balls)
{
    WINDOW* w = as_win(ui->status_win);
    if (!w)
        return;

    int rows, cols;
    getmaxyx(w, rows, cols);
    werase(w);
    box(w, 0, 0);

    int y = 1;
    int x = 2;
    if (y >= rows - 1)
        goto done;

    wattron(w, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(w, y++, x, "══ %s ══", name ? name : "(unknown)");
    wattroff(w, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(w, y++, x, "Index %d · Axes %d · Buttons %d · Hats %d · Balls %d",
              joy_idx, num_axes, num_buttons, num_hats, num_balls);
    y++;

    if (num_axes > 0 && y < rows - 1)
    {
        wattron(w, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(w, y++, x, "Axes");
        wattroff(w, COLOR_PAIR(3) | A_BOLD);
        int bar_cols = cols - 18;
        if (bar_cols < 8)
            bar_cols = 8;
        for (int i = 0; i < num_axes && y < rows - 1; ++i)
        {
            wmove(w, y, x);
            wprintw(w, "%2d %+6d ", i, (int)axes[i]);
            draw_axis_bar(w, axes[i], bar_cols);
            y++;
        }
        y++;
    }

    if (num_buttons > 0 && y < rows - 1)
    {
        wattron(w, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(w, y++, x, "Buttons");
        wattroff(w, COLOR_PAIR(2) | A_BOLD);

        int cx = x;
        int cy = y;
        for (int i = 0; i < num_buttons; ++i)
        {
            char cell[64];
            const char* label = (button_labels && button_labels[i] && button_labels[i][0])
                                    ? button_labels[i]
                                    : NULL;
            if (label)
                snprintf(cell, sizeof(cell), "%s%s ", buttons[i] ? "●" : "○", label);
            else
                snprintf(cell, sizeof(cell), "%s%02d ", buttons[i] ? "●" : "○", i);

            int disp_w = utf8_cols(cell);
            if (cx + disp_w >= cols - 2)
            {
                cx = x;
                cy++;
                if (cy >= rows - 1)
                    break;
            }
            if (buttons[i])
                wattron(w, COLOR_PAIR(2) | A_BOLD);
            mvwaddstr(w, cy, cx, cell);
            if (buttons[i])
                wattroff(w, COLOR_PAIR(2) | A_BOLD);
            cx += disp_w;
        }
        y = cy + 2;
    }

    if (num_hats > 0 && y < rows - 1)
    {
        wattron(w, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(w, y++, x, "Hats");
        wattroff(w, COLOR_PAIR(3) | A_BOLD);
        for (int i = 0; i < num_hats && y < rows - 1; ++i)
        {
            wmove(w, y, x);
            wprintw(w, "%2d ", i);
            draw_hat(w, hats[i]);
            y++;
        }
        y++;
    }

    if (num_balls > 0 && y < rows - 1)
    {
        wattron(w, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(w, y++, x, "Trackballs");
        wattroff(w, COLOR_PAIR(3) | A_BOLD);
        for (int i = 0; i < num_balls && y < rows - 1; ++i)
        {
            mvwprintw(w, y++, x, "%2d  Δx %+6d  Δy %+6d",
                      i, (int)balls[2 * i], (int)balls[2 * i + 1]);
        }
    }

    if (rows > 1)
        mvwprintw(w, rows - 1, x, "Ctrl-C / Esc to quit");
done:
    wnoutrefresh(w);
}

void test_ui_draw_events(TestUI* ui)
{
    WINDOW* w = as_win(ui->events_win);
    if (!w)
        return;

    int rows, cols;
    getmaxyx(w, rows, cols);
    werase(w);
    box(w, 0, 0);
    wattron(w, COLOR_PAIR(4));
    mvwprintw(w, 0, 2, " Events ");
    wattroff(w, COLOR_PAIR(4));

    int visible = rows - 2;
    if (visible < 1)
    {
        wnoutrefresh(w);
        return;
    }

    int start = 0;
    if (ui->log_len > visible)
        start = ui->log_len - visible;

    for (int i = 0; i < visible && (start + i) < ui->log_len; ++i)
    {
        int logical = start + i;
        int idx;
        if (ui->log_len < TEST_UI_LOG_CAP)
            idx = logical;
        else
            idx = (ui->log_head + logical) % TEST_UI_LOG_CAP;

        char line[TEST_UI_LOG_LEN];
        snprintf(line, sizeof(line), "%s", ui->log[idx]);
        /* crude byte clip; ok for log lines */
        int maxb = cols - 4;
        if (maxb > 0 && (int)strlen(line) > maxb)
            line[maxb] = '\0';
        mvwaddstr(w, 1 + i, 2, line);
    }
    wnoutrefresh(w);
}

bool test_ui_check_quit(TestUI* ui)
{
    bool quit = false;
    int ch;
    while ((ch = getch()) != ERR)
    {
        if (ch == 3 || ch == 27)
            quit = true;
        else if (ch == KEY_RESIZE)
            test_ui_relayout(ui);
    }
    return quit;
}

void test_ui_refresh(TestUI* ui)
{
    test_ui_draw_events(ui);
    doupdate();
    ui->dirty = false;
}
