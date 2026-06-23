#ifndef UI_H
#define UI_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

#define DIVIDER_COL        90
#define LOG_COL_START      (DIVIDER_COL + 2)
#define LOG_HEADER_ROW     0
#define LOG_START_ROW      2
#define UI_COL_MARGIN      2

#define UI_ROW_TITLE       0
#define UI_ROW_RES         2
#define UI_ROW_DWARVES     6

/* Horizontal split — dwarf list above, detail panel below */
#define UI_ROW_SPLIT       28
#define UI_ROW_DETAIL      29   /* detail panel starts here */
#define UI_ROW_CMDBAR      37

#define DWARF_LIST_ROWS    (UI_ROW_SPLIT - UI_ROW_DWARVES - 2)  /* ~20 rows */
#define LOG_VISIBLE_LINES  20
#define LOG_MAX_WRAPPED    512

extern int ui_selected_dwarf;
extern int ui_show_upgrades;
extern int ui_show_research;
extern int ui_show_breach;
extern int ui_show_prestige;
extern int ui_show_craft;
extern int ui_show_depth;
extern int ui_show_help;
extern int ui_show_achievements;

void ui_draw_all(Renderer *r, const GameState *state);
void ui_draw_titlebar(Renderer *r, const GameState *state);
void ui_draw_resources(Renderer *r, const GameState *state);
void ui_draw_dwarves(Renderer *r, const GameState *state);
void ui_draw_dwarf_detail(Renderer *r, const GameState *state);
void ui_draw_depth_confirm(Renderer *r, const GameState *state);
void ui_draw_help(Renderer *r);
void ui_draw_achievements(Renderer *r, const GameState *state);
void ui_draw_divider(Renderer *r);
void ui_draw_hline_full(Renderer *r, int row);
void ui_draw_eventlog(Renderer *r, const GameState *state);
void ui_draw_cmdbar(Renderer *r, const GameState *state);
void ui_draw_craft(Renderer *r, const GameState *state);

int  ui_dwarf_at_pixel(Renderer *r, const GameState *state, int x, int y);
void ui_log_scroll(int delta);
void ui_dwarf_scroll(int delta);
void ui_dwarf_select(int delta);

#endif