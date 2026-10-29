# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# libmain.so: sdl2-jstest or sdl3-jstest (SDL_MAJOR) with the shared code,
# against the prebuilt SDL module; no ncurses, so the window UI is the
# default mode. build-apk.sh puts src/ next to this file.
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := main
LOCAL_SRC_FILES := \
	src/sdl$(SDL_MAJOR)-jstest.c \
	src/common/cli.c \
	src/common/report.c \
	src/ui/graph_ui.c \
	src/ui/gui_nav.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src
LOCAL_CFLAGS := -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter \
	-DSDL_JSTEST_VERSION=\"$(SDL_JSTEST_VERSION)\" \
	-DSDL_JSTEST_BUILD_TYPE=\"Release\" \
	-DSDL$(SDL_MAJOR)_JSTEST_DATADIR=\"\"
LOCAL_SHARED_LIBRARIES := SDL$(SDL_MAJOR)
LOCAL_LDLIBS := -llog -landroid

include $(BUILD_SHARED_LIBRARY)
