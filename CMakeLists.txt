# SPDX-FileCopyrightText: 2015-2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-FileCopyrightText: 2018 Anthony J. Bentley <anthony@anjbe.name>
# SPDX-FileCopyrightText: 2021-2025 orbea <orbea@riseup.net>
# SPDX-FileCopyrightText: 2025 ABeltramo <beltramo.ale@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.10...4.1)

# Get version number (gamepkgstd VER-6)
if(NOT DEFINED PROJECT_VERSION_FULL)
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
endif()
string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" _version_numeric "${PROJECT_VERSION_FULL}")
project(sdl-jstest VERSION ${_version_numeric} LANGUAGES C)

option(WARNINGS "Switch on extra warnings" OFF)
option(WERROR "Turn warnings into errors" OFF)
option(BUILD_SDL_JSTEST "Build sdl1-jstest" ON)
option(BUILD_SDL2_JSTEST "Build sdl2-jstest" ON)
option(BUILD_SDL3_JSTEST "Build sdl3-jstest" ON)
option(SDL_JSTEST_NCURSES "Terminal UI through ncurses (else the window UI is the default)" ON)
option(BUILD_TESTS "Build the unit tests" ON)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if (WARNINGS)
  set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -pedantic -Wall -Wextra -Wshadow -Wcast-qual -Wconversion -Wmissing-prototypes -Wno-unused-parameter")
endif ()

if (WERROR)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif ()

# The build tree looks like an install: bin/ next to share/sdl-jstest/, so
# the binaries find gamecontrollerdb.txt relative to themselves (DIR-3).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(SDL_JSTEST_BUILD_DATADIR ${CMAKE_BINARY_DIR}/share/${PROJECT_NAME})
file(COPY external/sdl_gamecontrollerdb/gamecontrollerdb.txt
  DESTINATION ${SDL_JSTEST_BUILD_DATADIR})

# Code shared by the three binaries: options, startup report, window UI,
# terminal UI.
add_library(jstest_common STATIC
  src/common/cli.c
  src/common/cli.h
  src/common/report.c
  src/common/report.h
  src/ui/graph_ui.c
  src/ui/graph_ui.h
  src/ui/gui_nav.c
  src/ui/gui_nav.h)
target_include_directories(jstest_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(jstest_common PUBLIC
  SDL_JSTEST_VERSION="${PROJECT_VERSION_FULL}"
  SDL_JSTEST_BUILD_TYPE="${CMAKE_BUILD_TYPE}")

if (SDL_JSTEST_NCURSES)
  find_package(PkgConfig REQUIRED)
  pkg_search_module(NCURSES REQUIRED ncursesw ncurses IMPORTED_TARGET)
  target_sources(jstest_common PRIVATE src/ui/test_ui.c src/ui/test_ui.h)
  target_compile_definitions(jstest_common PUBLIC SDL_JSTEST_NCURSES)
  target_link_libraries(jstest_common PUBLIC PkgConfig::NCURSES)
endif ()

if (BUILD_SDL_JSTEST)
  find_package(SDL REQUIRED)
  if (NOT TARGET SDL::SDL)
    add_library(SDL::SDL INTERFACE IMPORTED)
    set_target_properties(SDL::SDL PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SDL_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES "${SDL_LIBRARY}")
  endif ()

  add_executable(sdl1-jstest src/sdl1-jstest.c)
  target_link_libraries(sdl1-jstest jstest_common SDL::SDL)
  target_compile_definitions(sdl1-jstest PRIVATE SDL1_JSTEST_DATADIR="${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}")

  install(TARGETS sdl1-jstest
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES doc/sdl1-jstest.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif ()

if (BUILD_SDL2_JSTEST)
  option(SDL_CUSTOM_SRC "Use a custom SDL source location (useful to better debug)" OFF)
  if (SDL_CUSTOM_SRC)
    SET(SDL_TEST OFF)
    SET(SDL_HIDAPI_JOYSTICK ON)
    add_subdirectory(${SDL_CUSTOM_SRC} ${CMAKE_CURRENT_BINARY_DIR}/sdl EXCLUDE_FROM_ALL)
  elseif (SDL2_INCLUDE_DIRS AND SDL2_LIBRARIES)
    # Given from outside (R36S: headers from the sysroot, a link stub).
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${SDL2_LIBRARIES}")
  else ()
    find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
  endif ()

  add_executable(sdl2-jstest src/sdl2-jstest.c src/utils/dualsense.h)
  target_link_libraries(sdl2-jstest jstest_common SDL2::SDL2)
  target_compile_definitions(sdl2-jstest PRIVATE SDL2_JSTEST_DATADIR="${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}")

  install(TARGETS sdl2-jstest
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES doc/sdl2-jstest.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif ()

if (BUILD_SDL3_JSTEST)
  find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

  add_executable(sdl3-jstest src/sdl3-jstest.c src/utils/dualsense.h)
  target_link_libraries(sdl3-jstest jstest_common SDL3::SDL3)
  target_compile_definitions(sdl3-jstest PRIVATE SDL3_JSTEST_DATADIR="${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}")

  install(TARGETS sdl3-jstest
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES doc/sdl3-jstest.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif ()

if (BUILD_SDL2_JSTEST OR BUILD_SDL3_JSTEST)
  install(FILES ${SDL_JSTEST_BUILD_DATADIR}/gamecontrollerdb.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
endif ()

# Launchers, icons, AppStream metainfo (gamepkgstd INT-6), per binary.
foreach (_prog sdl1-jstest sdl2-jstest sdl3-jstest)
  if ((_prog STREQUAL "sdl1-jstest" AND BUILD_SDL_JSTEST) OR
      (_prog STREQUAL "sdl2-jstest" AND BUILD_SDL2_JSTEST) OR
      (_prog STREQUAL "sdl3-jstest" AND BUILD_SDL3_JSTEST))
    install(FILES data/${_prog}.desktop
      DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES data/${_prog}.metainfo.xml
      DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
    install(FILES data/icons/${_prog}.svg
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
    # Generated by tools/make_icons.py (INT-4, INT-6).
    foreach (_size 16 32 48 64 128 256)
      install(FILES data/icons/${_size}/${_prog}.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_size}x${_size}/apps)
    endforeach ()
  endif ()
endforeach ()

if (BUILD_TESTS)
  enable_testing()
  add_executable(jstest_unit_tests tests/unit_tests.c)
  target_link_libraries(jstest_unit_tests jstest_common)
  add_test(NAME unit_tests COMMAND jstest_unit_tests)
endif ()

# EOF #
