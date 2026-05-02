#ifndef GAME_H
#define GAME_H

#include "../../include/asmoria.h"

#define TICK_MS 500

void game_init(GameState *state);
void game_update(GameState *state);

/* Try to load save file; fall back to game_init on failure.
   Pushes an appropriate event to the log in both cases. */
void game_load_or_init(GameState *state);

#endif /* GAME_H */