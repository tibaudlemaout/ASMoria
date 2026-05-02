#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdint.h>

#define WIN_W       1280
#define WIN_H       800
#define FONT_SIZE   16
#define GLYPH_W     10
#define GLYPH_H     18

/* Dwarf-hall colour palette */
#define COL_BG          0x0D0A07FF
#define COL_FG          0xD4B896FF
#define COL_GOLD        0xFFD700FF
#define COL_STONE       0x8899AAFF
#define COL_WOOD        0xA0724AFF
#define COL_FOOD        0x6DBF67FF
#define COL_MANA        0x8888FFFF
#define COL_DIM         0x554433FF
#define COL_ACCENT      0xFF9900FF

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font;
    int           glyph_w;
    int           glyph_h;
    int           win_w;        /* actual window width  (pixels) */
    int           win_h;        /* actual window height (pixels) */
} Renderer;

int  renderer_init(Renderer *r, const char *title);
void renderer_destroy(Renderer *r);

void renderer_clear(Renderer *r);
void renderer_present(Renderer *r);

void renderer_set_color_hex(Renderer *r, uint32_t rgba);

void renderer_draw_text(Renderer *r, int x, int y, uint32_t color,
                        const char *text);
void renderer_draw_text_grid(Renderer *r, int col, int row, uint32_t color,
                             const char *text);

/* Full-width horizontal rule */
void renderer_draw_hline(Renderer *r, int row, uint32_t color);

/* Horizontal rule spanning only cols [col_start .. col_end) */
void renderer_draw_hline_partial(Renderer *r, int row,
                                 int col_start, int col_end,
                                 uint32_t color);

void renderer_draw_box(Renderer *r, int col, int row, int cols, int rows,
                       uint32_t color);

/* Draw a pixel-precise border rect around the entire window */
void renderer_draw_screen_border(Renderer *r, uint32_t color, int thickness);

#endif /* RENDERER_H */