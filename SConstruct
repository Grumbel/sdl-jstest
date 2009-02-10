##  -*- python -*-
##  sdl-jstest - A Joystick Test Application For SDL
##  Copyright (C) 2000,2005 Ingo Ruhnke <grumbel@gmx.de>
##
##  This program is free software: you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation, either version 3 of the License, or
##  (at your option) any later version.
##  
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##  
##  You should have received a copy of the GNU General Public License
##  along with this program.  If not, see <http://www.gnu.org/licenses/>.

sdl_jstest_env = Environment()
sdl_jstest_env.Append(CCFLAGS=["-std=c99", "-Wall"])
sdl_jstest_env.Append(LIBS=["curses"])
sdl_jstest_env.ParseConfig('sdl-config --cflags --libs')
sdl_jstest_env.Program('sdl-jstest', ['sdl-jstest.c'])

# EOF #

