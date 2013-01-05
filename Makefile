# Set HB_PREFIX if you've got custom HarfBuzz installed somewhere else

HB_PREFIX?=/usr
CC=gcc
CFLAGS=--std=c99 -ggdb3 -Wall -Wextra -pedantic `sdl-config --cflags` `freetype-config --cflags` -I$(HB_PREFIX)/include
LDFLAGS=-L$(HB_PREFIX)/lib -Wl,-rpath -Wl,$(HB_PREFIX)/lib
LIBS=-lharfbuzz `sdl-config --libs` `freetype-config --libs`

all: ex-sdl-freetype-harfbuzz

ex-sdl-freetype-harfbuzz: ex-sdl-freetype-harfbuzz.c
	$(CC) $< $(CFLAGS) -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f ex-sdl-freetype-harfbuzz
