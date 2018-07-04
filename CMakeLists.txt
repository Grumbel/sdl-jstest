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
project(sdl-jstest VERSION 0.1.0)

include(GNUInstallDirs)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
find_package(SDL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

if(WARNINGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra -Wnon-virtual-dtor -Weffc++")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wshadow -Wcast-qual -Wconversion")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Winit-self -Wno-unused-parameter")
endif()

if(WERROR)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

link_directories(${SDL2_LIBRARY_DIRS})
add_executable(sdl2-jstest sdl2-jstest.c)
target_link_libraries(sdl2-jstest ${SDL2_LIBRARIES} ${CURSES_LIBRARIES})
target_include_directories(sdl2-jstest PUBLIC ${SDL2_INCLUDE_DIRS} ${CURSES_INCLUDE_DIRS})
target_compile_definitions(sdl2-jstest PUBLIC ${SDL2_CFLAGS_OTHER})

add_executable(sdl-jstest sdl-jstest.c)
target_link_libraries(sdl-jstest ${SDL_LIBRARY} ${CURSES_LIBRARIES})
target_include_directories(sdl-jstest PUBLIC ${SDL_INCLUDE_DIR} ${CURSES_INCLUDE_DIRS})

file(COPY sdl-jstest.1 sdl2-jstest.1
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY SDL_GameControllerDB/gamecontrollerdb.txt
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(doc ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.1 ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.1)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.1 ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.1
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

install(TARGETS sdl-jstest sdl2-jstest
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# EOF #
