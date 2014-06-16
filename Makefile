.PHONY: all clean

all: sdl-jstest sdl2-jstest sdl-jstest.1 sdl2-jstest.1

clean:
	rm -f sdl-jstest
	rm -f sdl-jstest.1

sdl-jstest: sdl-jstest.c
	$(CC) -o $@ $< -std=c99 -Wall `sdl-config --cflags --libs` -lcurses

sdl2-jstest: sdl2-jstest.c
	$(CC) -o $@ $< -std=c99 -Wall `pkg-config --cflags --libs sdl2` -lcurses

sdl-jstest.1: sdl-jstest.xml
	docbook2x-man --to-stdout $< > $@

sdl2-jstest.1: sdl2-jstest.xml
	docbook2x-man --to-stdout $< > $@

# EOF #
