# sdl-jstest - Joystick Test Program for SDL
# Copyright (C) 2015 Ingo Ruhnke <grumbel@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.0)
project(sdl-jstest)

set(TINYCMMC_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/external/tinycmmc/modules/")
find_package(tinycmmc CONFIG)
message(STATUS "tinycmmc module path: ${TINYCMMC_MODULE_PATH}")
list(APPEND CMAKE_MODULE_PATH ${TINYCMMC_MODULE_PATH})

option(WARNINGS "Switch on extra warnings" OFF)
option(WERROR "Turn warnings into errors" OFF)
option(BUILD_TESTS "Build test cases" OFF)
option(BUILD_NETWORK_TESTS "Build test cases using the network" OFF)
option(BUILD_SDL_JSTEST "Build sdl-jstest" ON)
option(BUILD_SDL2_JSTEST "Build sdl2-jstest" ON)

include(GetProjectVersion)
include(GNUInstallDirs)

add_definitions(-DSDL_JSTEST_VERSION="${PROJECT_VERSION}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

find_package(PkgConfig REQUIRED)

pkg_search_module(NCURSES REQUIRED ncursesw ncurses IMPORTED_TARGET)

if(WARNINGS)
  set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -pedantic -Wall -Wextra -Wshadow -Wcast-qual -Wconversion -Wmissing-prototypes -Wno-unused-parameter")
endif()

if(WERROR)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif()

if(BUILD_TESTS)
  # add 'make test' target, use 'make test ARGS="-V"' or 'ctest -V' for verbose
  enable_testing()
endif(BUILD_TESTS)

if(NOT BUILD_NETWORK_TESTS)
  set(APPSTREAM_UTIL_FLAGS --nonet)
endif()

if(BUILD_SDL_JSTEST)
  find_package(SDL REQUIRED)

  add_executable(sdl-jstest src/sdl-jstest.c)
  target_link_libraries(sdl-jstest
    SDL::SDL
    PkgConfig::NCURSES
    )

  configure_file(sdl-jstest.appdata.xml.in ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.appdata.xml)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.appdata.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

  if (BUILD_TESTS)
    add_test(NAME sdl-jstest.appdata.xml
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND appstream-util validate-relax ${APPSTREAM_UTIL_FLAGS} ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.appdata.xml)
  endif(BUILD_TESTS)

  file(COPY sdl-jstest.1
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

  install(TARGETS sdl-jstest
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(BUILD_SDL2_JSTEST)
  # not using find_package() as that includes some invalid include
  # directories that make cmake fail
  pkg_search_module(SDL2 REQUIRED sdl2 IMPORTED_TARGET)

  link_directories(${SDL2_LIBRARY_DIRS})
  add_executable(sdl2-jstest src/sdl2-jstest.c)
  target_link_libraries(sdl2-jstest
    PkgConfig::SDL2
    PkgConfig::NCURSES
    )
  target_compile_definitions(sdl2-jstest PUBLIC SDL2_JSTEST_DATADIR=\"${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}\")

  configure_file(sdl2-jstest.appdata.xml.in ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.appdata.xml)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.appdata.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

  if (BUILD_TESTS)
    add_test(NAME sdl2-jstest.appdata.xml
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND appstream-util validate-relax ${APPSTREAM_UTIL_FLAGS} ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.appdata.xml)
  endif(BUILD_TESTS)

  file(COPY sdl2-jstest.1
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  file(COPY external/sdl_gamecontrollerdb/gamecontrollerdb.txt
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gamecontrollerdb.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

  install(TARGETS sdl2-jstest
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# EOF #
