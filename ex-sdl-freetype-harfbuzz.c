#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <SDL.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#define NUM_EXAMPLES 3

/* tranlations courtesy of google */
const char *texts[NUM_EXAMPLES] = {
    "Ленивый рыжий кот",
    "كسول الزنجبيل القط",
    "懶惰的姜貓",
};

const int text_directions[NUM_EXAMPLES] = {
    HB_DIRECTION_LTR,
    HB_DIRECTION_RTL,
    HB_DIRECTION_TTB,
};

/* XXX: These are not correct, though it doesn't seem to break anything
 *      regardless of their value. */
const char *languages[NUM_EXAMPLES] = {
    "en",
    "ar",
    "ch",
};

const hb_script_t scripts[NUM_EXAMPLES] = {
    HB_SCRIPT_LATIN,
    HB_SCRIPT_ARABIC,
    HB_SCRIPT_HAN,
};

enum {
    ENGLISH=0, ARABIC, CHINESE
};

typedef struct _spanner_baton_t { // assumes 32bpp surface
    uint32_t *pixels; // set to y-start of the glyph.
    uint32_t *first_pixel, *last_pixel; // bounds check
    uint32_t color;
    uint32_t pitch;
    uint32_t rshift;
    uint32_t gshift;
    uint32_t bshift;
    uint32_t ashift;
} spanner_baton_t;

/* google this */
#ifndef unlikely
#define unlikely
#endif

/* This spanner is write only, suitable for write-only mapped buffers,
   but can cause dark streaks where glyphs overlap, like in arabic scripts.

   Note how spanners don't clip against surface width - resize the window
   and see what it leads to. */
void spanner_wo(int y, int count, const FT_Span* spans, void *user) {
    spanner_baton_t *baton = (spanner_baton_t *) user;
    uint32_t *scanline = baton->pixels - y * ( (int) baton->pitch / 4 );
    if (unlikely scanline < baton->first_pixel)
        return;

    for (int i = 0; i < count; i++) {
        uint32_t color =
            ((spans[i].coverage/2) << baton->rshift) |
            ((spans[i].coverage/2) << baton->gshift) |
            ((spans[i].coverage/2) << baton->bshift);

        uint32_t *start = scanline + spans[i].x;
        if (unlikely start + spans[i].len > baton->last_pixel)
            return;

        for (int x = 0; x < spans[i].len; x++)
            *start++ = color;
    }
}

/* This spanner does read/modify/write, trading performance for accuracy.
   Suitable for when artifacts mostly do come up and annoy.
   This might be optimized if one does rmw only for some values of x.
   But since the whole buffer has to be rw anyway, and the previous value
   is probably still in the cache, there's little point to. */
void spanner_rw(int y, int count, const FT_Span* spans, void *user) {
    spanner_baton_t *baton = (spanner_baton_t *) user;
    uint32_t *scanline = baton->pixels - y * ( (int) baton->pitch / 4 );
    if (unlikely scanline < baton->first_pixel)
        return;

    for (int i = 0; i < count; i++) {
        uint32_t color =
            ((spans[i].coverage/2)  << baton->rshift) |
            ((spans[i].coverage/2) << baton->gshift) |
            ((spans[i].coverage/2) << baton->bshift);

        uint32_t *start = scanline + spans[i].x;
        if (unlikely start + spans[i].len > baton->last_pixel)
            return;

        for (int x = 0; x < spans[i].len; x++)
            *start++ |= color;
    }
}

FT_SpanFunc spanner = spanner_rw;

void ftfdump(FT_Face ftf) {
    for(int i=0; i<ftf->num_charmaps; i++) {
        printf("%d: %s %s %c%c%c%c plat=%hu id=%hu\n", i,
            ftf->family_name,
            ftf->style_name,
            ftf->charmaps[i]->encoding >>24,
           (ftf->charmaps[i]->encoding >>16 ) & 0xff,
           (ftf->charmaps[i]->encoding >>8) & 0xff,
           (ftf->charmaps[i]->encoding) & 0xff,
            ftf->charmaps[i]->platform_id,
            ftf->charmaps[i]->encoding_id
        );
    }
}

/*  See http://www.microsoft.com/typography/otspec/name.htm
    for a list of some possible platform-encoding pairs.
    We're interested in 0-3 aka 3-1 - UCS-2.
    Otherwise, fail. If a font has some unicode map, but lacks
    UCS-2 - it is a broken or irrelevant font. What exactly 
    Freetype will select on face load (it promises most wide 
    unicode, and if that will be slower that UCS-2 - left as 
    an excercise to check. */
int force_ucs2_charmap(FT_Face ftf) {
    for(int i = 0; i < ftf->num_charmaps; i++)
        if ((  (ftf->charmaps[i]->platform_id == 0)
            && (ftf->charmaps[i]->encoding_id == 3))
           || ((ftf->charmaps[i]->platform_id == 3)
            && (ftf->charmaps[i]->encoding_id == 1)))
                return FT_Set_Charmap(ftf, ftf->charmaps[i]);
    return -1;
}

int main () {
    int ptSize = 50*64;
    int device_hdpi = 72;
    int device_vdpi = 72;

    /* Init freetype */
    FT_Library ft_library;
    assert(!FT_Init_FreeType(&ft_library));

    /* Load our fonts */
    FT_Face ft_face[NUM_EXAMPLES];
    assert(!FT_New_Face(ft_library, "fonts/DejaVuSerif.ttf", 0, &ft_face[ENGLISH]));
    assert(!FT_Set_Char_Size(ft_face[ENGLISH], 0, ptSize, device_hdpi, device_vdpi ));
    ftfdump(ft_face[ENGLISH]); // wonderful world of encodings ...
    force_ucs2_charmap(ft_face[ENGLISH]); // which we ignore.

    assert(!FT_New_Face(ft_library, "fonts/amiri-0.104/amiri-regular.ttf", 0, &ft_face[ARABIC]));
    assert(!FT_Set_Char_Size(ft_face[ARABIC], 0, ptSize, device_hdpi, device_vdpi ));
    ftfdump(ft_face[ARABIC]);
    force_ucs2_charmap(ft_face[ARABIC]);

    assert(!FT_New_Face(ft_library, "fonts/fireflysung-1.3.0/fireflysung.ttf", 0, &ft_face[CHINESE]));
    assert(!FT_Set_Char_Size(ft_face[CHINESE], 0, ptSize, device_hdpi, device_vdpi ));
    ftfdump(ft_face[CHINESE]);
    force_ucs2_charmap(ft_face[CHINESE]);

    /* Get our harfbuzz font/face structs */
    hb_font_t *hb_ft_font[NUM_EXAMPLES];
    hb_face_t *hb_ft_face[NUM_EXAMPLES];
    hb_ft_font[ENGLISH] = hb_ft_font_create(ft_face[ENGLISH], NULL);
    hb_ft_face[ENGLISH] = hb_ft_face_create(ft_face[ENGLISH], NULL);
    hb_ft_font[ARABIC]  = hb_ft_font_create(ft_face[ARABIC] , NULL);
    hb_ft_face[ARABIC]  = hb_ft_face_create(ft_face[ARABIC] , NULL);
    hb_ft_font[CHINESE] = hb_ft_font_create(ft_face[CHINESE], NULL);
    hb_ft_face[CHINESE] = hb_ft_face_create(ft_face[CHINESE], NULL);

    /** Setup our SDL window **/
    int width      = 800;
    int height     = 600;
    int videoFlags = SDL_SWSURFACE | SDL_RESIZABLE | SDL_DOUBLEBUF;
    int bpp        = 32;

    /* Initialize our SDL window */
    if(SDL_Init(SDL_INIT_VIDEO) < 0)   {
        fprintf(stderr, "Failed to initialize SDL");
        return -1;
    }

    SDL_WM_SetCaption("\"Simple\" SDL+FreeType+HarfBuzz Example", "\"Simple\" SDL+FreeType+HarfBuzz Example");

    SDL_Surface *screen;
    screen = SDL_SetVideoMode(width, height, bpp, videoFlags);

    /* Enable key repeat, just makes it so we don't have to worry about fancy
     * scanboard keyboard input and such */
    SDL_EnableKeyRepeat(300, 130);
    SDL_EnableUNICODE(1);

    /* Create an SDL image surface we can draw to */
    SDL_Surface *sdl_surface = SDL_CreateRGBSurface (0, width, height, 32, 0,0,0,0);

    /* Our main event/draw loop */
    int done = 0;
    int frame = 0;
    while (!done) {
        /* Clear our surface */
        SDL_FillRect( sdl_surface, NULL, 0 );
        SDL_LockSurface(sdl_surface);

        int x = 0;
        int y = 50;
        for (int i=0; i < NUM_EXAMPLES; ++i) {
            /* Create a buffer for harfbuzz to use */
            hb_buffer_t *buf = hb_buffer_create();
            hb_buffer_set_direction(buf, text_directions[i]); /* or LTR */
            hb_buffer_set_script(buf, scripts[i]); /* see hb-unicode.h */
            hb_buffer_set_language(buf, hb_language_from_string(languages[i], strlen(languages[i])));

            /* Layout the text */
            hb_buffer_add_utf8(buf, texts[i], strlen(texts[i]), 0, strlen(texts[i]));
            hb_shape(hb_ft_font[i], buf, NULL, 0);

            unsigned int         glyph_count;
            hb_glyph_info_t     *glyph_info   = hb_buffer_get_glyph_infos(buf, &glyph_count);
            hb_glyph_position_t *glyph_pos    = hb_buffer_get_glyph_positions(buf, &glyph_count);

            int string_width_in_pixels = 0;
            int string_height_in_pixels = 0;
            for (unsigned j=0; j < glyph_count; ++j) {
                string_width_in_pixels += glyph_pos[j].x_advance/64;
                string_height_in_pixels += glyph_pos[j].y_advance/64;
            }
            /* width/height are in fact shift thus w/o last glyph, so height is 0
                for ltr/rtl scripts and width is 0 for ttb (han (and height is negative)),
                so they are just enough for crude justification. */
            if (!frame)
                printf("ex %d string in pixels = %dx%d\n", i, string_width_in_pixels, string_height_in_pixels);

            if (i == ENGLISH) { x = 20; }                                   /* left justify */
            if (i == ARABIC)  { x = width - string_width_in_pixels -20; }   /* right justify */
            if (i == CHINESE) { x = width/2 - string_width_in_pixels/2; }   /* center */

            spanner_baton_t stuffbaton;
            stuffbaton.pixels = NULL;
            stuffbaton.first_pixel = sdl_surface->pixels;
            stuffbaton.last_pixel = (uint32_t *) (((uint8_t *) sdl_surface->pixels) + sdl_surface->pitch*sdl_surface->h);
            stuffbaton.pitch = sdl_surface->pitch;
            stuffbaton.rshift = sdl_surface->format->Rshift;
            stuffbaton.gshift = sdl_surface->format->Gshift;
            stuffbaton.bshift = sdl_surface->format->Bshift;

            FT_Raster_Params ftr_params;
            ftr_params.target = 0;
            ftr_params.flags = FT_RASTER_FLAG_DIRECT | FT_RASTER_FLAG_AA;
            ftr_params.user = &stuffbaton;
            ftr_params.gray_spans = spanner; // throw it in
            ftr_params.black_spans = 0;
            ftr_params.bit_set = 0;
            ftr_params.bit_test = 0;

            FT_Error fterr;

            for (unsigned j=0; j < glyph_count; ++j) {
                if ((fterr = FT_Load_Glyph(ft_face[i], glyph_info[j].codepoint, 0))) {
                    printf("load %08x failed fterr=%d.\n",  glyph_info[j].codepoint, fterr);
                } else {
                    if (ft_face[i]->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
                        printf("glyph->format = %4s\n", (char *)&ft_face[i]->glyph->format);
                    } else {
                        int gx = x + (glyph_pos[j].x_offset/64);
                        int gy = y - (glyph_pos[j].y_offset/64);
                        stuffbaton.pixels = (uint32_t *)(((uint8_t *) sdl_surface->pixels) + gy * sdl_surface->pitch) + gx;

                        if ((fterr = FT_Outline_Render(ft_library, &ft_face[i]->glyph->outline, &ftr_params)))
                            printf("FT_Outline_Render() failed err=%d\n", fterr);
                    }
                }

                x += glyph_pos[j].x_advance/64;
                y -= glyph_pos[j].y_advance/64;
            }

            hb_buffer_destroy(buf);
            y += 75;
        }
        SDL_UnlockSurface(sdl_surface);

        /* Blit our new image to our visible screen */

        SDL_BlitSurface(sdl_surface, NULL, screen, NULL);
        SDL_Flip(screen);

        /* Handle SDL events */
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        done = 1;
                    }
                    break;

                case SDL_VIDEORESIZE:
                    width = event.resize.w;
                    height = event.resize.h;
                    screen = SDL_SetVideoMode(event.resize.w, event.resize.h, bpp, videoFlags);
                    if (!screen) {
                        fprintf(stderr, "Could not get a surface after resize: %s\n", SDL_GetError( ));
                        exit(-1);
                    }
                    /*  Recreate an SDL image surface we can draw to. */
                    SDL_FreeSurface(sdl_surface);
                    sdl_surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0 ,0, 0);
                    break;
            }
        }

        SDL_Delay(150);
        frame++;
    }

    /* Cleanup */
    for (int i=0; i < NUM_EXAMPLES; ++i) {
        hb_font_destroy(hb_ft_font[i]);
        hb_face_destroy(hb_ft_face[i]);
    }

    FT_Done_FreeType(ft_library);

    SDL_FreeSurface(sdl_surface);
    SDL_Quit();

    return 0;
}
