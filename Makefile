CC=gcc
CFLAGS=--std=c99 -g -O2 -Wall --pedantic `pkg-config --cflags freetype2` `sdl2-config --cflags`
LIBS=-lcairo -lharfbuzz -lharfbuzz-icu `pkg-config --libs freetype2` `sdl2-config --libs`

all: ex-sdl-cairo-freetype-harfbuzz

ex-sdl-cairo-freetype-harfbuzz: ex-sdl-cairo-freetype-harfbuzz.c
	$(CC) $< $(CFLAGS) -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f ex-sdl-cairo-freetype-harfbuzz
