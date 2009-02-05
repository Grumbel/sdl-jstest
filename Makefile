sdl-jstest: sdl-jstest.c
	$(CC) -o $@ $< -std=c99 -Wall `sdl-config --cflags --libs` -lcurses

# EOF #
