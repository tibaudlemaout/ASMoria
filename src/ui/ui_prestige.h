#ifndef UI_PRESTIGE_H
#define UI_PRESTIGE_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

extern int ui_prestige_cursor;

void ui_draw_prestige(Renderer *r, const GameState *state);
void ui_prestige_move(int delta);
int  ui_prestige_selected(void);

#endif