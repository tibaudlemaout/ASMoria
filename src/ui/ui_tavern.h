#ifndef UI_TAVERN_H
#define UI_TAVERN_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

extern int ui_tavern_cursor;
extern int ui_show_tavern;

void ui_draw_tavern(Renderer *r, const GameState *state);
void ui_tavern_move(int delta);
int  ui_tavern_selected(void);

#endif