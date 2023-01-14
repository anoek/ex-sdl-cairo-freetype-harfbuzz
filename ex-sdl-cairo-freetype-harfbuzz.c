#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <SDL.h>
#include <SDL_image.h>

#include <fontconfig/fontconfig.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftadvanc.h>
#include <freetype/ftsnames.h>
#include <freetype/tttables.h>

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-icu.h> /* Alternatively you can use hb-glib.h */

#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

#define NUM_EXAMPLES 3
#define FONT_SIZE 40

const char *texts[NUM_EXAMPLES] = {
    "This is some english text",
    "هذه بعض النصوص العربية",
    "這是一些中文",
};

const int text_directions[NUM_EXAMPLES] = {
    HB_DIRECTION_LTR,
    HB_DIRECTION_RTL,
    HB_DIRECTION_TTB,
};

/* XXX: I don't know if these are correct or not, it doesn't seem to break anything
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

int main () {
    
    /* Init font config */
    FcInit(); //initializes Fontconfig
	FcConfig* config = FcInitLoadConfigAndFonts();
    assert(config);
    
    FcPattern* pat = FcNameParse((const FcChar8*)"DejaVu Serif");
    
    /* Increase the possible matches */
    FcConfigSubstitute(config, pat, FcMatchPattern);
	FcDefaultSubstitute(pat);
    
	FcResult result;

	FcPattern* font = FcFontMatch(config, pat, &result);
    assert(font);
    
    /* Will be freed with font */
    FcChar8* file = NULL; 
    
    assert(FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch);
    
    /* Init freetype */
    FT_Library ft_library;
    assert(!FT_Init_FreeType(&ft_library));

    /* Load our fonts */
    FT_Face ft_face[NUM_EXAMPLES];
    assert(!FT_New_Face(ft_library, (char *)file, 0, &ft_face[ENGLISH]));
    assert(!FT_Set_Char_Size(ft_face[ENGLISH],FONT_SIZE*64, FONT_SIZE*64,0,0));
    assert(!FT_New_Face(ft_library, "fonts/amiri-0.104/amiri-regular.ttf", 0, &ft_face[ARABIC]));
    assert(!FT_Set_Char_Size(ft_face[ARABIC],FONT_SIZE*64, FONT_SIZE*64,0,0 ));
    assert(!FT_New_Face(ft_library, "fonts/fireflysung-1.3.0/fireflysung.ttf", 0, &ft_face[CHINESE]));
    assert(!FT_Set_Char_Size(ft_face[CHINESE],FONT_SIZE*64, FONT_SIZE*64,0,0 ));
    
    /* Cleanup font config */
    FcPatternDestroy(font);
    FcPatternDestroy(pat);
    FcConfigDestroy(config);
    FcFini();

    /* Get our cairo font structs */
    cairo_font_face_t *cairo_ft_face[NUM_EXAMPLES];
    cairo_ft_face[ENGLISH] = cairo_ft_font_face_create_for_ft_face(ft_face[ENGLISH], 0);
    cairo_ft_face[ARABIC]  = cairo_ft_font_face_create_for_ft_face(ft_face[ARABIC], 0);
    cairo_ft_face[CHINESE] = cairo_ft_font_face_create_for_ft_face(ft_face[CHINESE], 0);

    /* Get our harfbuzz font/face structs */
    hb_font_t *hb_ft_font[NUM_EXAMPLES];
    hb_face_t *hb_ft_face[NUM_EXAMPLES];
    hb_ft_font[ENGLISH] = hb_ft_font_create(ft_face[ENGLISH], NULL);
    hb_ft_face[ENGLISH] = hb_ft_face_create(ft_face[ENGLISH], NULL);
    hb_ft_font[ARABIC]  = hb_ft_font_create(ft_face[ARABIC] , NULL);
    hb_ft_face[ARABIC]  = hb_ft_face_create(ft_face[ARABIC] , NULL);
    hb_ft_font[CHINESE] = hb_ft_font_create(ft_face[CHINESE], NULL);
    hb_ft_face[CHINESE] = hb_ft_face_create(ft_face[CHINESE], NULL);

    /* Setup our SDL window */
    int width      = 800;
    int height     = 600;
    int videoFlags =   SDL_WINDOW_RESIZABLE  ;

    /* Initialize our SDL window */
    if(SDL_Init(SDL_INIT_VIDEO) < 0)   { 
        fprintf(stderr, "Failed to initialize SDL");
        return -1; 
    } 

    SDL_Window *screen = SDL_CreateWindow(
        "\"Simple\" SDL+Cairo+FreeType+HarfBuzz Example",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width,
        height,
        videoFlags
    );

    SDL_Renderer *renderer = SDL_CreateRenderer( 
        screen,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    /* Create an SDL image surface we can hand to cairo to draw to */
    SDL_Surface *sdl_surface = SDL_CreateRGBSurface (
        videoFlags, 
        width, 
        height, 
        32,
        0x00ff0000,
        0x0000ff00,
        0x000000ff,
        0
    );

    /* Our main event/draw loop */
    int done = 0;
    while (!done) {
        /* Clear our surface */
        SDL_FillRect( sdl_surface, NULL, 0 );

        /* Create a cairo surface which will write directly to the sdl surface */
        cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data (
            (unsigned char *)sdl_surface->pixels,
            CAIRO_FORMAT_RGB24,
            sdl_surface->w,
            sdl_surface->h,
            sdl_surface->pitch
        );
        cairo_t *cr = cairo_create(cairo_surface);


        /*******************************************/
        /*** Draw our text to the cairo context ***/ 
        /*******************************************/
        int x = 0;
        int y = 50;
        for (int i=0; i < NUM_EXAMPLES; ++i) {
            /* Create a buffer for harfbuzz to use */
            hb_buffer_t *buf = hb_buffer_create();

            //alternatively you can use hb_buffer_set_unicode_funcs(buf, hb_glib_get_unicode_funcs());
            hb_buffer_set_unicode_funcs(buf, hb_icu_get_unicode_funcs());
            hb_buffer_set_direction(buf, text_directions[i]); /* or LTR */
            hb_buffer_set_script(buf, scripts[i]); /* see hb-unicode.h */
            hb_buffer_set_language(buf, hb_language_from_string(languages[i], -1));

            /* Layout the text */
            hb_buffer_add_utf8(buf, texts[i], strlen(texts[i]), 0, strlen(texts[i]));
            hb_shape(hb_ft_font[i], buf, NULL, 0);

            /* Hand the layout to cairo to render */
            unsigned int         glyph_count;
            hb_glyph_info_t     *glyph_info   = hb_buffer_get_glyph_infos(buf, &glyph_count);
            hb_glyph_position_t *glyph_pos    = hb_buffer_get_glyph_positions(buf, &glyph_count);
            cairo_glyph_t       *cairo_glyphs = malloc(sizeof(cairo_glyph_t) * glyph_count);

            unsigned int string_width_in_pixels = 0;
            for (int i=0; i < glyph_count; ++i) {
                string_width_in_pixels += glyph_pos[i].x_advance/64;
            }
            if (i == ENGLISH) { x = 20; }                                   /* left justify */
            if (i == ARABIC)  { x = width - string_width_in_pixels -20; }   /* right justify */
            if (i == CHINESE) { x = width/2 - string_width_in_pixels/2; }   /* center */

            for (int i=0; i < glyph_count; ++i) {
                cairo_glyphs[i].index = glyph_info[i].codepoint;
                cairo_glyphs[i].x = x + (glyph_pos[i].x_offset/64.0);
                cairo_glyphs[i].y = y - (glyph_pos[i].y_offset/64.0);
                x += glyph_pos[i].x_advance/64.0;
                y -= glyph_pos[i].y_advance/64.0;
            }

            cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 1.0);
            cairo_set_font_face(cr, cairo_ft_face[i]);
            cairo_set_font_size(cr, FONT_SIZE);
            cairo_show_glyphs(cr, cairo_glyphs, glyph_count);

            free(cairo_glyphs);
            hb_buffer_destroy(buf);
            y += 75;
        }

        /******************************************************************************/
        /*** Cleanup our cairo surface, copy it to the screen, deal with SDL events ***/
        /******************************************************************************/

        /* Blit our new image to our visible screen */ 
        SDL_SetRenderDrawColor(renderer,235, 52, 146,0);
        SDL_RenderClear(renderer);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer,sdl_surface);
        SDL_RenderCopy(renderer,texture,NULL,NULL);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(texture);

        /* We're now done with our cairo surface */
        cairo_surface_destroy(cairo_surface);
        cairo_destroy(cr);


        /* Handle SDL events. Wait for the first event, process any others that
         * may also be pending. If you don't want to pause your program waiting
         * for events, use SDL_PollEvent instead. */
        SDL_Event event;
        if (SDL_WaitEvent(&event)) {
            do {
                switch (event.type) {
                    case SDL_KEYDOWN:
                        if (event.key.keysym.sym == SDLK_ESCAPE) {
                            done = 1;
                        }
                        break;

                    case SDL_QUIT:
                        done = 1;
                        break;

                    case SDL_WINDOWEVENT:
                        switch (event.window.event) {
                            case SDL_WINDOWEVENT_SIZE_CHANGED:
                                width = event.window.data1;
                                height = event.window.data2;
                                break;
                            case SDL_WINDOWEVENT_CLOSE:
                                event.type = SDL_QUIT;
                                SDL_PushEvent(&event);
                                break;
                        }
                        /* Create an SDL image surface we can hand to cairo to draw to */
                        SDL_FreeSurface(sdl_surface);
                        sdl_surface = SDL_CreateRGBSurface (
                            videoFlags,
                            width,
                            height,
                            32,
                            0x00ff0000,
                            0x0000ff00,
                            0x000000ff,
                            0
                        );
                        break;
                }
            } while(SDL_PollEvent(&event));
        }
    }

    /* Cleanup */
    for (int i=0; i < NUM_EXAMPLES; ++i) {
        cairo_font_face_destroy(cairo_ft_face[i]);
        hb_font_destroy(hb_ft_font[i]);
        hb_face_destroy(hb_ft_face[i]);
    }

    FT_Done_FreeType(ft_library);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    SDL_Quit();

    return 0;
}
