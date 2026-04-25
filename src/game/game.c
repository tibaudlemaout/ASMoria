#include "game.h"
#include "../../include/asmoria.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stddef.h>

/* =========================================================
 * game_init
 * Set up a brand-new GameState: zero everything, seed the
 * RNG, and spawn one starter miner dwarf.
 * ========================================================= */
void game_init(GameState *state) {
    memset(state, 0, sizeof(*state));
    fprintf(stderr, "sizeof GameState  = %zu\n", sizeof(GameState));
    fprintf(stderr, "sizeof EventLog   = %zu\n", sizeof(EventLog));
    fprintf(stderr, "sizeof EventRecord= %zu\n", sizeof(EventRecord));
    fprintf(stderr, "offset resources  = %zu\n", offsetof(GameState, resources));
    fprintf(stderr, "offset dwarves    = %zu\n", offsetof(GameState, dwarves));
    fprintf(stderr, "offset rng        = %zu\n", offsetof(GameState, rng));
    fprintf(stderr, "offset tick       = %zu\n", offsetof(GameState, tick));
    fprintf(stderr, "offset event_log  = %zu\n", offsetof(GameState, event_log));
    fprintf(stderr, "offset evtlog head= %zu\n", offsetof(GameState, event_log.head));

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
    fprintf(stderr, "tick=%llu count=%u head=%u\n",
            (unsigned long long)state->tick,
            state->event_log.count,
            state->event_log.head);
}