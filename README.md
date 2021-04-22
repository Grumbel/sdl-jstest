sdl-jstest
==========

`sdl-jstest` and `sdl2-jstest` are simple programs that lets you find
out how many joysticks SDL or SDL2 detected on your system, how many
axes, buttons, hats and balls they have each. They also lets you test
the joysticks by displaying the events they send or by displaying
their current button, axis, hat or ball state. sdl-jstest is
especially useful if you want to test your `SDL_LINUX_JOYSTICK`
configuration.


Requirements
------------

sdl-jstest requires the following packages to build:

* cmake
* libsdl1.2-dev
* libsdl2-dev
* libncurses5-dev


Submodules
----------

To fetch submodule dependencies:

    git submodule init
    git submodule update


Compilation
-----------

To compile type:

    mkdir build
    cd build
    cmake ..
    make

To install (optional):

    make install

The `DESTDIR` and `PREFIX` can be configured as follows:

    make install DESTDIR=/tmp/pingus

    cmake .. -DCMAKE_INSTALL_PREFIX:PATH=/usr


Cross-compilation to Win32
--------------------------

MXE (http://mxe.cc) is needed. Follow its installation instructions,
but you can build it with just the needed packages for sdl-jstest:

    make gcc cmake sdl sdl2 ncurses pdcurses

This still takes quite a while.

After MXE installation finishes, for sdl-jstest compilation type:

    mkdir build
    cd build
    i686-w64-mingw32.static-cmake ..
    make


Configuration
-------------

The SDL behaviour of what axis is treated as a hat and which as normal
axis can be configured with the environment variable
`SDL_LINUX_JOYSTICK`:

 * ftp://ptah.lnf.kth.se/pub/misc/sdl-env-vars

`SDL_LINUX_JOYSTICK`:
	Special joystick configuration string for linux. The format is
	"name numaxes numhats numballs"
	where name is the name string of the joystick (possibly in single
	quotes), and the rest are the number of axes, hats and balls
	respectively.

A simple example would be:

    $ export SDL_LINUX_JOYSTICK="'Xbox Gamepad (userspace driver)' 6 0 0"

Which makes all axis be handled as normal axis and none as hat, as
most games don't use hats.
