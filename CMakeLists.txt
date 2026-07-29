# SPDX-FileCopyrightText: 2015-2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-FileCopyrightText: 2018 Anthony J. Bentley <anthony@anjbe.name>
# SPDX-FileCopyrightText: 2021-2025 orbea <orbea@riseup.net>
# SPDX-FileCopyrightText: 2025 ABeltramo <beltramo.ale@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.10...4.1)
project(sdl-jstest)

option(WARNINGS "Switch on extra warnings" OFF)
option(WERROR "Turn warnings into errors" OFF)
option(BUILD_SDL_JSTEST "Build sdl-jstest" ON)
option(BUILD_SDL2_JSTEST "Build sdl2-jstest" ON)
option(BUILD_SDL3_JSTEST "Build sdl3-jstest" ON)

include(GNUInstallDirs)

# Get version number
if(NOT DEFINED PROJECT_VERSION_FULL)
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
endif()
add_definitions(-DSDL_JSTEST_VERSION="${PROJECT_VERSION_FULL}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

find_package(PkgConfig REQUIRED)

pkg_search_module(NCURSES REQUIRED ncursesw ncurses IMPORTED_TARGET)

if (WARNINGS)
  set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -pedantic -Wall -Wextra -Wshadow -Wcast-qual -Wconversion -Wmissing-prototypes -Wno-unused-parameter")
endif ()

if (WERROR)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif ()

if (BUILD_SDL_JSTEST)
  find_package(SDL REQUIRED)

  add_executable(sdl-jstest src/sdl-jstest.c)
  target_link_libraries(sdl-jstest
    SDL::SDL
    PkgConfig::NCURSES
  )

  file(COPY doc/sdl-jstest.1
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

  install(TARGETS sdl-jstest
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  install(FILES
    data/sdl-jstest.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif ()

if (BUILD_SDL2_JSTEST)
  find_package(PkgConfig)
  option(SDL_CUSTOM_SRC "Use a custom SDL source location (useful to better debug)" OFF)
  if (SDL_CUSTOM_SRC)
    SET(SDL_TEST OFF)
    SET(SDL_HIDAPI_JOYSTICK ON)
    add_subdirectory(${SDL_CUSTOM_SRC} ${CMAKE_CURRENT_BINARY_DIR}/sdl EXCLUDE_FROM_ALL)
  else ()
    find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
  endif ()

  add_executable(sdl2-jstest src/sdl2-jstest.c src/utils/dualsense.h)
  target_link_libraries(sdl2-jstest
    SDL2::SDL2
    PkgConfig::NCURSES
  )
  target_compile_definitions(sdl2-jstest PUBLIC SDL2_JSTEST_DATADIR=\"${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}\")

  file(COPY doc/sdl2-jstest.1
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  file(COPY external/sdl_gamecontrollerdb/gamecontrollerdb.txt
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gamecontrollerdb.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

  install(TARGETS sdl2-jstest
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  install(FILES
    data/sdl2-jstest.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif ()

if (BUILD_SDL3_JSTEST)
  find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

  add_executable(sdl3-jstest src/sdl3-jstest.c src/utils/dualsense.h)
  target_link_libraries(sdl3-jstest
    SDL3::SDL3
    PkgConfig::NCURSES
  )
  target_compile_definitions(sdl3-jstest PUBLIC SDL3_JSTEST_DATADIR=\"${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}\")

  file(COPY doc/sdl3-jstest.1
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  file(COPY external/sdl_gamecontrollerdb/gamecontrollerdb.txt
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gamecontrollerdb.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl3-jstest.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

  install(TARGETS sdl3-jstest
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  install(FILES
    data/sdl3-jstest.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif ()

# EOF #
