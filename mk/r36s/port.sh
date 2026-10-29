#!/bin/bash
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# PortMaster entry for @PROGRAM@; the work is in sdl-jstest/launch.sh,
# shared by the three programs.

exec bash "$(dirname "$0")/sdl-jstest/launch.sh" @PROGRAM@ "$@"
