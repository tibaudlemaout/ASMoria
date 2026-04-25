#include "game.h"
#include "../../include/asmoria.h"
#include <string.h>
#include <time.h>

/* =========================================================
 * game_init
 * Set up a brand-new GameState: zero everything, seed the
 * RNG, and spawn one starter miner dwarf.
 * ========================================================= */
void game_init(GameState *state) {
    memset(state, 0, sizeof(*state));

    /* Seed RNG from system time */
    asm_rng_seed(state, (uint64_t)time(NULL) ^ 0xA5710A1A000ULL);

    /* Starter resources */
    state->resources.gold  = 10;
    state->resources.stone = 50;
    state->resources.wood  = 30;
    state->resources.food  = 20;
    state->depth           = 1;

    /* Spawn one starter dwarf as a miner */
    state->dwarves[0].alive   = 1;
    state->dwarves[0].job     = JOB_MINER;
    state->dwarves[0].morale  = 80;
    state->dwarves[0].fatigue = 0;
    state->dwarves[0].xp     = 0;
}

/* =========================================================
 * game_update
 * Thin C wrapper — all real logic lives in ASM.
 * ========================================================= */
void game_update(GameState *state) {
    asm_tick(state);
}