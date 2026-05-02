#ifndef GAME_H
#define GAME_H

#include "../../include/asmoria.h"

/* Milliseconds per game tick (16ms ≈ 60tps logical, display uncapped) */
#define TICK_MS 500

/* Initialize a fresh GameState with one starter dwarf */
void game_init(GameState *state);

/* Called once per TICK_MS by the main loop — delegates to asm_tick */
void game_update(GameState *state);

#endif /* GAME_H */