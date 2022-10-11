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
option(BUILD_SDL_JSTEST "Build sdl-jstest" ON)
option(BUILD_SDL2_JSTEST "Build sdl2-jstest" ON)

include(GetProjectVersion)
include(GNUInstallDirs)

add_definitions(-DSDL_JSTEST_VERSION="${PROJECT_VERSION}")

include(GNUInstallDirs)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)

if(WARNINGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra -Wnon-virtual-dtor -Weffc++")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wshadow -Wcast-qual -Wconversion")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Winit-self -Wno-unused-parameter")
endif()

if(WERROR)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

if (BUILD_TESTS)
  enable_testing()
endif(BUILD_TESTS)

if(BUILD_SDL_JSTEST)
  find_package(SDL REQUIRED)

  add_executable(sdl-jstest src/sdl-jstest.c)
  target_link_libraries(sdl-jstest ${SDL_LIBRARY} ${CURSES_LIBRARIES})
  target_include_directories(sdl-jstest PUBLIC ${SDL_INCLUDE_DIR} ${CURSES_INCLUDE_DIRS})

  configure_file(sdl-jstest.appdata.xml.in ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.appdata.xml)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.appdata.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

  if (BUILD_TESTS)
    add_test(NAME sdl-jstest.appdata.xml
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND appstream-util validate-relax ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.appdata.xml)
  endif(BUILD_TESTS)

  file(COPY sdl-jstest.1
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

  install(TARGETS sdl-jstest
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(BUILD_SDL2_JSTEST)
  find_package(PkgConfig REQUIRED)
  pkg_search_module(SDL2 REQUIRED sdl2)

  link_directories(${SDL2_LIBRARY_DIRS})
  add_executable(sdl2-jstest src/sdl2-jstest.c)
  target_link_libraries(sdl2-jstest ${SDL2_LIBRARIES} ${CURSES_LIBRARIES})
  target_include_directories(sdl2-jstest PUBLIC ${SDL2_INCLUDE_DIRS} ${CURSES_INCLUDE_DIRS})
  target_compile_definitions(sdl2-jstest PUBLIC ${SDL2_CFLAGS_OTHER})
  target_compile_definitions(sdl2-jstest PUBLIC SDL2_JSTEST_DATADIR=\"${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}\")

  configure_file(sdl2-jstest.appdata.xml.in ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.appdata.xml)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.appdata.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

  if (BUILD_TESTS)
    add_test(NAME sdl2-jstest.appdata.xml
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMAND appstream-util validate-relax ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.appdata.xml)
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
