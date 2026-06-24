#ifndef UI_CARAVAN_H
#define UI_CARAVAN_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

extern int ui_show_caravan;

void    ui_draw_caravan(Renderer *r, const GameState *state);
void    ui_caravan_field_move(int delta);   /* LEFT/RIGHT — switch field */
void    ui_caravan_adjust(int delta);       /* UP/DOWN   — change value  */
int     ui_caravan_cur_workers(void);
int     ui_caravan_cur_guards(void);
int64_t ui_caravan_cur_food(void);

#endif