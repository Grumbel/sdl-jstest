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

/* Approximate terminal columns for a UTF-8 string (■/□/─ count as 1). */
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
    /* Clear stdscr once so newwin panes are not left under a stale screen. */
    clear();
    refresh();
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
    /* Paint the empty event pane immediately (status filled by caller). */
    test_ui_draw_events(ui);
    doupdate();
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

/* SDL_HAT_* layout (same in SDL 1/2/3):
 *   UP=0x01  RIGHT=0x02  DOWN=0x04  LEFT=0x08  (diagonals are ORed)
 * Drawn as a spatial d-pad so directions match the stick, not a U/D/L/R list.
 */
static int draw_hat(WINDOW* w, int y, int x, uint8_t hat, int max_y)
{
    const char* u = (hat & 0x01) ? "▲" : "·";
    const char* r = (hat & 0x02) ? "▶" : "·";
    const char* d = (hat & 0x04) ? "▼" : "·";
    const char* l = (hat & 0x08) ? "◀" : "·";
    const char* c = (hat == 0) ? "·" : "●";

    if (y <= max_y)
        mvwprintw(w, y, x, "  %s", u);
    if (y + 1 <= max_y)
        mvwprintw(w, y + 1, x, "%s %s %s", l, c, r);
    if (y + 2 <= max_y)
        mvwprintw(w, y + 2, x, "  %s", d);
    return 3;
}


void test_ui_draw_joystick(TestUI* ui,
                           const char* name,
                           int joy_idx,
                           const int16_t* axes, int num_axes,
                           const uint8_t* buttons, int num_buttons,
                           const char* const* button_labels,
                           const TestUIButtonGroup* button_groups, int num_groups,
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

        /* Build group list: explicit groups, or one group for all buttons */
        TestUIButtonGroup fallback = { 0, num_buttons };
        const TestUIButtonGroup* groups = button_groups;
        int ng = num_groups;
        if (!groups || ng <= 0)
        {
            groups = &fallback;
            ng = 1;
        }

        for (int g = 0; g < ng && y < rows - 1; ++g)
        {
            int begin = groups[g].begin;
            int end_i = groups[g].end;
            if (begin < 0)
                begin = 0;
            if (end_i > num_buttons)
                end_i = num_buttons;
            if (begin >= end_i)
                continue;

            int cx = x;
            int cy = y;
            for (int i = begin; i < end_i; ++i)
            {
                char cell[64];
                const char* mark = buttons[i] ? "■" : "□";
                const char* label = (button_labels && button_labels[i] && button_labels[i][0])
                                        ? button_labels[i]
                                        : NULL;
                if (label)
                    snprintf(cell, sizeof(cell), " %s %s ", mark, label);
                else
                    snprintf(cell, sizeof(cell), " %s %02d ", mark, i);

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
            y = cy + 1; /* next group on a new row */
        }
        y++;
    }

    if (num_hats > 0 && y < rows - 1)
    {
        wattron(w, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(w, y++, x, "Hats");
        wattroff(w, COLOR_PAIR(3) | A_BOLD);
        for (int i = 0; i < num_hats && y < rows - 1; ++i)
        {
            mvwprintw(w, y, x, "%2d", i);
            y += draw_hat(w, y, x + 4, hats[i], rows - 2);
            y++; /* gap between hats */
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
        mvwprintw(w, rows - 1, x, "Esc: back to menu  ·  Ctrl-C: exit");
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

TestUIAction test_ui_poll_action(TestUI* ui)
{
    TestUIAction action = TEST_UI_CONTINUE;
    int ch;
    while ((ch = getch()) != ERR)
    {
        if (ch == 3) /* Ctrl-C */
            action = TEST_UI_QUIT;
        else if (ch == 27) /* Esc */
            action = TEST_UI_BACK;
        else if (ch == KEY_RESIZE)
            test_ui_relayout(ui);
    }
    return action;
}

int test_ui_menu_select(TestUI* ui, const char* title,
                        const char* const* items, int n_items)
{
    int selected = 0;
    if (n_items < 0)
        n_items = 0;

    for (;;)
    {
        WINDOW* w = as_win(ui->status_win);
        WINDOW* ew = as_win(ui->events_win);
        if (!w)
            return -1;

        int rows, cols;
        getmaxyx(w, rows, cols);
        (void)cols;
        werase(w);
        box(w, 0, 0);

        int y = 1;
        int x = 2;
        wattron(w, COLOR_PAIR(1) | A_BOLD);
        mvwprintw(w, y++, x, "%s", title ? title : "Select device");
        wattroff(w, COLOR_PAIR(1) | A_BOLD);
        mvwprintw(w, y++, x, "↑/↓ or j/k  ·  Enter select  ·  Esc quit");
        y++;

        if (n_items == 0)
        {
            mvwprintw(w, y++, x, "(no devices detected — plug one in and wait)");
        }
        else
        {
            int list_top = y;
            int visible = rows - list_top - 2;
            if (visible < 1)
                visible = 1;
            int start = 0;
            if (selected >= start + visible)
                start = selected - visible + 1;
            if (selected < start)
                start = selected;

            for (int i = 0; i < visible && start + i < n_items; ++i)
            {
                int idx = start + i;
                int row = list_top + i;
                if (idx == selected)
                    wattron(w, COLOR_PAIR(4) | A_BOLD);
                mvwprintw(w, row, x, "%s %2d  %s",
                          idx == selected ? "▶" : " ",
                          idx,
                          items[idx] ? items[idx] : "?");
                if (idx == selected)
                    wattroff(w, COLOR_PAIR(4) | A_BOLD);
            }
        }

        if (rows > 1)
            mvwprintw(w, rows - 1, x, "sdl-jstest");

        wnoutrefresh(w);
        if (ew)
        {
            werase(ew);
            box(ew, 0, 0);
            wattron(ew, COLOR_PAIR(4));
            mvwprintw(ew, 0, 2, " Events ");
            wattroff(ew, COLOR_PAIR(4));
            mvwprintw(ew, 1, 2, "Waiting for a selection…");
            wnoutrefresh(ew);
        }
        doupdate();

        /* Blocking-ish: wait briefly for a key */
        timeout(100);
        int ch = getch();
        timeout(0); /* restore nodelay */
        nodelay(stdscr, TRUE);

        if (ch == ERR)
            continue;
        if (ch == KEY_RESIZE)
        {
            test_ui_relayout(ui);
            continue;
        }
        if (ch == 3) /* Ctrl-C */
            return -1;
        if (ch == 27) /* Esc */
            return -1;
        if (ch == KEY_UP || ch == 'k' || ch == 'K')
        {
            if (n_items > 0)
                selected = (selected + n_items - 1) % n_items;
            continue;
        }
        if (ch == KEY_DOWN || ch == 'j' || ch == 'J')
        {
            if (n_items > 0)
                selected = (selected + 1) % n_items;
            continue;
        }
        if (ch == '\n' || ch == KEY_ENTER || ch == ' ')
        {
            if (n_items > 0)
                return selected;
            continue;
        }
        /* digit jump */
        if (ch >= '0' && ch <= '9' && n_items > 0)
        {
            int d = ch - '0';
            if (d < n_items)
                selected = d;
        }
    }
}

void test_ui_refresh(TestUI* ui)
{
    if (ui->status_win)
        wnoutrefresh(as_win(ui->status_win));
    test_ui_draw_events(ui);
    doupdate();
    ui->dirty = false;
}
