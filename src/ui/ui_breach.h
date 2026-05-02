#ifndef UI_BREACH_H
#define UI_BREACH_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

void ui_draw_breach(Renderer *r, const GameState *state);

/* Index of selected guard in breach panel (for feeding) */
extern int ui_breach_selected_guard;

void ui_breach_select(int delta);

#endif