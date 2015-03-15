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

* libsdl1.2-dev
* libsdl2-dev
* libncurses5-dev

For building the manpages:

* docbook2x


Compilation
-----------

To compile just type:

    $ make

To rebuild the man-pages:

    $ make doc


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
