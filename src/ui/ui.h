#ifndef UI_H
#define UI_H

#include "../render/renderer.h"
#include "../../include/asmoria.h"

/* =========================================================
 * ASMoria UI
 * All UI is text-based. The screen is divided into named
 * panels drawn each frame from live GameState data.
 *
 * Layout (80-col terminal aesthetic, scaled to window):
 *
 *  ┌──────────────────────────────────────────────────┐
 *  │  ASMoria                          Tick: 000000   │  row 0   (title bar)
 *  ├──────────────────────────────────────────────────┤
 *  │  RESOURCES                                       │  row 2   (resource panel)
 *  │  Gold: 0   Stone: 0   Wood: 0   Food: 0          │  row 3
 *  ├──────────────────────────────────────────────────┤
 *  │  DWARVES (0/64)                                  │  row 5   (dwarf panel)
 *  │  ...                                             │
 *  ├──────────────────────────────────────────────────┤
 *  │  LOG                                             │  bottom  (event log)
 *  └──────────────────────────────────────────────────┘
 * ========================================================= */

/* Row constants for panel anchors */
#define UI_ROW_TITLE     0
#define UI_ROW_RES       2
#define UI_ROW_DWARVES   5
#define UI_COL_MARGIN    2

void ui_draw_all(Renderer *r, const GameState *state);
void ui_draw_titlebar(Renderer *r, const GameState *state);
void ui_draw_resources(Renderer *r, const GameState *state);
void ui_draw_dwarves(Renderer *r, const GameState *state);

#endif /* UI_H */