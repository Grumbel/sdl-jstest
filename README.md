sdl-jstest
==========

`sdl1-jstest`, `sdl2-jstest`, and `sdl3-jstest` are simple programs that lets you find
out how many joysticks SDL, SDL2, or SDL3 detected on your system, how many
axes, buttons, hats and balls they have each. They also lets you test
the joysticks by displaying the events they send or by displaying
their current button, axis, hat or ball state. sdl1-jstest is
especially useful if you want to test your `SDL_LINUX_JOYSTICK`
configuration.


Screenshots
-----------

![sdl-jstest 0.3.0](doc/screenshots/screenshot-0.3.0.png)


Requirements
------------

sdl-jstest requires the following packages to build:

* cmake
* libsdl1.2-dev
* libsdl2-dev
* libsdl3-dev
* libncurses5-dev


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

