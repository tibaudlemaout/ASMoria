#ifndef UI_WONDER_H
#define UI_WONDER_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

extern int ui_show_wonder;
extern int ui_wonder_cursor;

void ui_draw_wonder(Renderer *r, const GameState *state);
void ui_wonder_move(int delta);
int  ui_wonder_selected(void);

#endif