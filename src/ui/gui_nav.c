// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/gui_nav.h"

void gui_hold_set(GuiHold* hold, bool down, uint32_t now)
{
    if (down && !hold->down)
        hold->since = now;
    hold->down = down;
}

float gui_hold_progress(const GuiHold* hold, uint32_t now, uint32_t duration)
{
    if (!hold->down || duration == 0) return 0.0f;
    uint32_t held = now - hold->since;
    if (held >= duration) return 1.0f;
    return (float)held / (float)duration;
}

GuiAction gui_stick_axis(GuiStick* stick, int value)
{
    int dir = value <= -GUI_STICK_THRESHOLD ? -1 : value >= GUI_STICK_THRESHOLD ? 1 : 0;
    if (dir == 0) {
        stick->dir = 0;
        return GUI_NONE;
    }
    /* The next step needs the stick back in the centre first. */
    if (stick->dir != 0) return GUI_NONE;
    stick->dir = dir;
    return dir < 0 ? GUI_UP : GUI_DOWN;
}
