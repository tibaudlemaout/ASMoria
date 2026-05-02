#ifndef UI_RESEARCH_H
#define UI_RESEARCH_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

extern int ui_research_cursor;

void ui_draw_research(Renderer *r, const GameState *state);
void ui_research_move(int delta);
int  ui_research_selected(void);

#endif