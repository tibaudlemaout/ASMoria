#ifndef UI_CRAFT_H
#define UI_CRAFT_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

extern int ui_craft_cursor;

void ui_draw_craft(Renderer *r, const GameState *state);
void ui_craft_move(int delta);
int  ui_craft_selected(void);

#endif