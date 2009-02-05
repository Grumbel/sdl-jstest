##  -*- python -*-
##  $Id: windstille_main.hpp 1097 2005-09-23 16:01:59Z grumbel $
##   __      __ __             ___        __   __ __   __
##  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
##  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
##   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
##    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
##         \/          \/      \/    \/                         \/
##  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
##
##  This program is free software; you can redistribute it and/or
##  modify it under the terms of the GNU General Public License
##  as published by the Free Software Foundation; either version 2
##  of the License, or (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
## 
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
##  02111-1307, USA.
##

Import('conf_env')

sdl_jstest_env = conf_env.Clone()
sdl_jstest_env.Append(CCFLAGS=["-std=c99", "-Wall"])
sdl_jstest_env.Append(LIBS=["curses"])
sdl_jstest_env.ParseConfig('sdl-config --cflags --libs')
sdl_jstest_env.Program('sdl-jstest', ['sdl-jstest.c'])

# EOF #

