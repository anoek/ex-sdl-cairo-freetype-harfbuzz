CC=gcc
CFLAGS=--std=c99 -g -O2 -Wall --pedantic `pkg-config --cflags` `sdl2-config --cflags`
#LDFLAGS=`icu-config --ldflags`
LIBS=-lcairo -lharfbuzz -lharfbuzz-icu `pkg-config --cflags --libs freetype2` `pkg-config --libs` `sdl2-config --libs`

all: ex-sdl-cairo-freetype-harfbuzz

ex-sdl-cairo-freetype-harfbuzz: ex-sdl-cairo-freetype-harfbuzz.c
	$(CC) $< $(CFLAGS) -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f ex-sdl-cairo-freetype-harfbuzz
