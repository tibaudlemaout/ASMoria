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

/* How many screen rows the log panel has */
#define LOG_VISIBLE_LINES  38

/* Max wrapped lines we buffer per frame (generous upper bound) */
#define LOG_MAX_WRAPPED    512

void ui_draw_all(Renderer *r, const GameState *state);
void ui_draw_titlebar(Renderer *r, const GameState *state);
void ui_draw_resources(Renderer *r, const GameState *state);
void ui_draw_dwarves(Renderer *r, const GameState *state);
void ui_draw_divider(Renderer *r);
void ui_draw_eventlog(Renderer *r, const GameState *state);

/* Scroll the event log: +1 = scroll up (older), -1 = scroll down (newer) */
void ui_log_scroll(int delta);

#endif /* UI_H */