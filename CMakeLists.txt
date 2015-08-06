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

cmake_minimum_required(VERSION 2.8)

include(GNUInstallDirs)

find_package(Curses REQUIRED)
find_package(SDL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2)

find_program(DOCBOOK2XMAN docbook2x-man)
if(DOCBOOK2XMAN)
  message(STATUS "docbook2x-man: ${DOCBOOK2XMAN}")
else()
  message(STATUS "docbook2x-man: not found")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

if(WARNINGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra -Wnon-virtual-dtor -Weffc++")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wshadow -Wcast-qual -Wconversion")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Winit-self -Wno-unused-parameter")
endif()

if(WERROR)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

add_executable(sdl2-jstest sdl2-jstest.c)
target_link_libraries(sdl2-jstest ${SDL2_LIBRARIES} ${CURSES_LIBRARIES})
target_include_directories(sdl2-jstest PUBLIC ${SDL2_INCLUDE_DIRS} ${CURSES_INCLUDE_DIR})
target_compile_definitions(sdl2-jstest PUBLIC ${SDL2_CFLAGS_OTHER})

add_executable(sdl-jstest sdl-jstest.c)
target_link_libraries(sdl-jstest ${SDL_LIBRARY} ${CURSES_LIBRARIES})
target_include_directories(sdl-jstest PUBLIC ${SDL_INCLUDE_DIR} ${CURSES_INCLUDE_DIR})

if(DOCBOOK2XMAN)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.1
    COMMAND ${DOCBOOK2XMAN} ${CMAKE_CURRENT_SOURCE_DIR}/sdl-jstest.xml
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sdl-jstest.xml
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.1
    COMMAND ${DOCBOOK2XMAN} ${CMAKE_CURRENT_SOURCE_DIR}/sdl2-jstest.xml
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sdl2-jstest.xml
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
else()
  file(COPY sdl-jstest.1 sdl2-jstest.1
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

add_custom_target(doc ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.1 ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.1)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sdl-jstest.1 ${CMAKE_CURRENT_BINARY_DIR}/sdl2-jstest.1
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

install(TARGETS sdl-jstest sdl2-jstest
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# EOF #
