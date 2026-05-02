#include "renderer.h"
#include <stdio.h>
#include <string.h>

static void set_color(Renderer *r, uint32_t rgba) {
    SDL_SetRenderDrawColor(r->renderer,
        (rgba >> 24) & 0xFF,
        (rgba >> 16) & 0xFF,
        (rgba >>  8) & 0xFF,
        (rgba      ) & 0xFF);
}

int renderer_init(Renderer *r, const char *title) {
    memset(r, 0, sizeof(*r));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "[renderer] SDL_Init: %s\n", SDL_GetError());
        return -1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "[renderer] TTF_Init: %s\n", TTF_GetError());
        return -1;
    }

    r->window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!r->window) {
        fprintf(stderr, "[renderer] CreateWindow: %s\n", SDL_GetError());
        return -1;
    }

    r->renderer = SDL_CreateRenderer(r->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!r->renderer) {
        fprintf(stderr, "[renderer] CreateRenderer: %s\n", SDL_GetError());
        return -1;
    }

    /* Try fonts in order */
    r->font = TTF_OpenFont("assets/fonts/terminus.ttf", FONT_SIZE);
    if (!r->font)
        r->font = TTF_OpenFont(
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", FONT_SIZE);
    if (!r->font)
        r->font = TTF_OpenFont(
            "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
            FONT_SIZE);
    if (!r->font) {
        fprintf(stderr, "[renderer] No monospace font found: %s\n",
                TTF_GetError());
        return -1;
    }

    int adv = 0;
    TTF_GlyphMetrics(r->font, 'M', NULL, NULL, NULL, NULL, &adv);
    r->glyph_w = (adv > 0) ? adv : GLYPH_W;
    r->glyph_h = TTF_FontHeight(r->font);

    SDL_GetWindowSize(r->window, &r->win_w, &r->win_h);
    return 0;
}

void renderer_destroy(Renderer *r) {
    if (r->font)     TTF_CloseFont(r->font);
    if (r->renderer) SDL_DestroyRenderer(r->renderer);
    if (r->window)   SDL_DestroyWindow(r->window);
    TTF_Quit();
    SDL_Quit();
}

void renderer_clear(Renderer *r) {
    /* Update cached window size every frame so resize works */
    SDL_GetWindowSize(r->window, &r->win_w, &r->win_h);
    set_color(r, COL_BG);
    SDL_RenderClear(r->renderer);
}

void renderer_present(Renderer *r) {
    SDL_RenderPresent(r->renderer);
}

void renderer_set_color_hex(Renderer *r, uint32_t rgba) {
    set_color(r, rgba);
}

void renderer_draw_text(Renderer *r, int x, int y, uint32_t color,
                        const char *text) {
    if (!text || !*text) return;
    SDL_Color c = {
        (color >> 24) & 0xFF,
        (color >> 16) & 0xFF,
        (color >>  8) & 0xFF,
        (color      ) & 0xFF
    };
    SDL_Surface *surf = TTF_RenderText_Blended(r->font, text, c);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r->renderer, surf);
    SDL_FreeSurface(surf);
    if (!tex) return;
    SDL_Rect dst = { x, y, 0, 0 };
    SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
    SDL_RenderCopy(r->renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

void renderer_draw_text_grid(Renderer *r, int col, int row, uint32_t color,
                             const char *text) {
    renderer_draw_text(r, col * r->glyph_w, row * r->glyph_h, color, text);
}

void renderer_draw_hline(Renderer *r, int row, uint32_t color) {
    set_color(r, color);
    SDL_Rect line = { 0, row * r->glyph_h + r->glyph_h / 2,
                      r->win_w, 1 };
    SDL_RenderFillRect(r->renderer, &line);
}

void renderer_draw_hline_partial(Renderer *r, int row,
                                 int col_start, int col_end,
                                 uint32_t color) {
    set_color(r, color);
    SDL_Rect line = {
        col_start * r->glyph_w,
        row * r->glyph_h + r->glyph_h / 2,
        (col_end - col_start) * r->glyph_w,
        1
    };
    SDL_RenderFillRect(r->renderer, &line);
}

void renderer_draw_box(Renderer *r, int col, int row, int cols, int rows,
                       uint32_t color) {
    set_color(r, color);
    SDL_Rect rect = {
        col  * r->glyph_w, row  * r->glyph_h,
        cols * r->glyph_w, rows * r->glyph_h
    };
    SDL_RenderDrawRect(r->renderer, &rect);
}

void renderer_draw_screen_border(Renderer *r, uint32_t color, int thickness) {
    set_color(r, color);
    /* top */
    SDL_Rect t = { 0, 0, r->win_w, thickness };
    SDL_RenderFillRect(r->renderer, &t);
    /* bottom */
    SDL_Rect b = { 0, r->win_h - thickness, r->win_w, thickness };
    SDL_RenderFillRect(r->renderer, &b);
    /* left */
    SDL_Rect l = { 0, 0, thickness, r->win_h };
    SDL_RenderFillRect(r->renderer, &l);
    /* right */
    SDL_Rect ri = { r->win_w - thickness, 0, thickness, r->win_h };
    SDL_RenderFillRect(r->renderer, &ri);
}
