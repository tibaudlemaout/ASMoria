#ifndef UI_H
#define UI_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

/* =========================================================
 * Screen layout (grid columns)
 *
 *  col 0                    col 62  col 64
 *  |                        |       |
 *  [ LEFT PANEL - 62 cols  ]|[ LOG  - right ]
 *
 *  Left panel:  title bar, resources, dwarves
 *  Right panel: event log (scrolling, newest at bottom)
 *  Divider:     vertical rule at col DIVIDER_COL
 * ========================================================= */

#define DIVIDER_COL     62
#define LOG_COL_START   (DIVIDER_COL + 2)
#define LOG_HEADER_ROW  0
#define LOG_START_ROW   2
#define UI_COL_MARGIN   2

/* Left panel row anchors */
#define UI_ROW_TITLE    0
#define UI_ROW_RES      2
#define UI_ROW_DWARVES  6

/* How many log lines fit on screen */
#define LOG_VISIBLE_LINES  38

void ui_draw_all(Renderer *r, const GameState *state);
void ui_draw_titlebar(Renderer *r, const GameState *state);
void ui_draw_resources(Renderer *r, const GameState *state);
void ui_draw_dwarves(Renderer *r, const GameState *state);
void ui_draw_divider(Renderer *r);
void ui_draw_eventlog(Renderer *r, const GameState *state);

#endif /* UI_H */