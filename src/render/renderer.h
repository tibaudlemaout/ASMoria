#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdint.h>

/* =========================================================
 * ASMoria Renderer
 * Thin SDL2 + SDL_ttf wrapper. All drawing is text-based;
 * the renderer exposes glyph-level and string-level calls.
 * ========================================================= */

#define WIN_W       1024
#define WIN_H       768
#define FONT_SIZE   16
#define GLYPH_W     10   /* approximate, updated after font load */
#define GLYPH_H     18

/* Dwarf-hall colour palette */
#define COL_BG          0x0D0A07FF   /* deep charcoal-brown        */
#define COL_FG          0xD4B896FF   /* warm parchment             */
#define COL_GOLD        0xFFD700FF   /* gold                       */
#define COL_STONE       0x8899AAFF   /* slate                      */
#define COL_WOOD        0xA0724AFF   /* timber                     */
#define COL_FOOD        0x6DBF67FF   /* moss green                 */
#define COL_MANA        0x8888FFFF   /* arcane blue                */
#define COL_DIM         0x554433FF   /* dimmed text / borders      */
#define COL_ACCENT      0xFF9900FF   /* highlight / selected       */

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font;
    int           glyph_w;
    int           glyph_h;
} Renderer;

/* Lifecycle */
int  renderer_init(Renderer *r, const char *title);
void renderer_destroy(Renderer *r);

/* Frame control */
void renderer_clear(Renderer *r);
void renderer_present(Renderer *r);

/* Drawing primitives */
void renderer_set_color_hex(Renderer *r, uint32_t rgba);

/* Draw a string at pixel position (x, y) */
void renderer_draw_text(Renderer *r, int x, int y, uint32_t color,
                        const char *text);

/* Draw a string at grid position (col, row) */
void renderer_draw_text_grid(Renderer *r, int col, int row, uint32_t color,
                             const char *text);

/* Draw a horizontal rule across the full window width */
void renderer_draw_hline(Renderer *r, int row, uint32_t color);

/* Draw a box outline in grid coords */
void renderer_draw_box(Renderer *r, int col, int row, int cols, int rows,
                       uint32_t color);

#endif /* RENDERER_H */