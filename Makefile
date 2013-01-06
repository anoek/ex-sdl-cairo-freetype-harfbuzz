# make PREFIX=/some/where if you've got custom HarfBuzz installed somewhere

PREFIX?=/usr
export PKG_CONFIG_PATH:=$(PREFIX)/lib/pkgconfig
CC=gcc
CFLAGS=--std=c99 -ggdb3 -Wall -Wextra -pedantic \
    `pkg-config sdl --cflags` \
    `pkg-config freetype2 --cflags` \
    `pkg-config harfbuzz --cflags`
LIBS=\
    `pkg-config sdl --libs` \
    `pkg-config freetype2 --libs` \
    `pkg-config harfbuzz --libs`

# so as not to have to use LD_LIBRARY_PATH when prefix is custom
LDFLAGS=-Wl,-rpath -Wl,$(PREFIX)/lib

all: ex-sdl-freetype-harfbuzz

ex-sdl-freetype-harfbuzz: ex-sdl-freetype-harfbuzz.c
	$(CC) $< $(CFLAGS) -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f ex-sdl-freetype-harfbuzz
