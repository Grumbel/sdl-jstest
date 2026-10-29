// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/* Navigation of the window UI without a keyboard (handhelds, Android): the
   SDL-independent part. Each binary turns its events into GuiActions. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum GuiAction {
    GUI_NONE = 0,
    GUI_UP,
    GUI_DOWN,
    GUI_CONFIRM,
    GUI_PICK,     /* a menu row by pointer or digit key; index separately */
    GUI_BACK,     /* leave the screen: device view → menu, menu → quit */
    GUI_QUIT      /* leave the program */
} GuiAction;

/* A device view is left by holding a button, so that every button stays
   testable: the gamepad's Back / Select for GUI_HOLD_MS (SDL2 / SDL3), any
   button for GUI_HOLD_ANY_MS (SDL1, which knows no button names). */
#define GUI_HOLD_MS 1000u
#define GUI_HOLD_ANY_MS 2000u

typedef struct GuiHold {
    bool down;
    uint32_t since;
} GuiHold;

void gui_hold_set(GuiHold* hold, bool down, uint32_t now);
/* 0 when not held, rising to 1 when the hold completes. */
float gui_hold_progress(const GuiHold* hold, uint32_t now, uint32_t duration);

/* Stick → one step per deflection past the threshold. */
#define GUI_STICK_THRESHOLD 16000

typedef struct GuiStick {
    int dir;      /* -1 up, 0 centre, 1 down */
} GuiStick;

GuiAction gui_stick_axis(GuiStick* stick, int value);

#ifdef __cplusplus
}
#endif
