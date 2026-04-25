#ifndef UI_H
#define UI_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

#define DIVIDER_COL        62
#define LOG_COL_START      (DIVIDER_COL + 2)
#define LOG_HEADER_ROW     0
#define LOG_START_ROW      2
#define UI_COL_MARGIN      2

#define UI_ROW_TITLE       0
#define UI_ROW_RES         2
#define UI_ROW_DWARVES     6

/* Command bar sits at the bottom of the left panel */
#define UI_ROW_CMDBAR      40

#define LOG_VISIBLE_LINES  36
#define LOG_MAX_WRAPPED    512

/* Selected dwarf index, -1 = none */
extern int ui_selected_dwarf;

void ui_draw_all(Renderer *r, const GameState *state);
void ui_draw_titlebar(Renderer *r, const GameState *state);
void ui_draw_resources(Renderer *r, const GameState *state);
void ui_draw_dwarves(Renderer *r, const GameState *state);
void ui_draw_divider(Renderer *r);
void ui_draw_eventlog(Renderer *r, const GameState *state);
void ui_draw_cmdbar(Renderer *r, const GameState *state);

/* Returns dwarf index (0-based) for a click at pixel (x,y), or -1 */
int  ui_dwarf_at_pixel(Renderer *r, const GameState *state, int x, int y);

void ui_log_scroll(int delta);

#endif /* UI_H */