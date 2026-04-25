#ifndef UI_UPGRADES_H
#define UI_UPGRADES_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

extern int ui_upgr_cursor;

void ui_draw_upgrades(Renderer *r, const GameState *state);
void ui_upgr_move(int delta);
int  ui_upgr_selected(void);

#endif