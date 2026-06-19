#ifndef UI_BREACH_H
#define UI_BREACH_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

extern int ui_breach_cursor_col;
extern int ui_breach_cursor_row;

void ui_draw_breach(Renderer *r, const GameState *state);
void ui_breach_move(int dc, int dr);

#endif