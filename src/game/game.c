#include "game.h"
#include "save.h"
#include "../../include/asmoria.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

void game_init(GameState *state) {
    memset(state, 0, sizeof(*state));
    asm_rng_seed(state, (uint64_t)time(NULL) ^ 0xA5710A1A000ULL);

    state->resources.gold  = 10;
    state->resources.stone = 50;
    state->resources.wood  = 30;
    state->resources.food  = 20;
    state->depth           = 1;
    state->pending.code    = 0xFF;

    state->dwarves[0].alive    = 1;
    state->dwarves[0].job      = JOB_MINER;
    state->dwarves[0].morale   = 80;
    state->dwarves[0].fatigue  = 0;
    state->dwarves[0].prev_job = JOB_IDLE;
    
}

void game_update(GameState *state) {
    asm_tick(state);

    if (state->tick % AUTOSAVE_INTERVAL == 0)
        save_game(state);
}

void game_load_or_init(GameState *state) {
    SaveResult r = load_game(state);

    if (r == SAVE_OK) {
        asm_event_push(state, 0x4B, EVT_MILESTONE, 0xFF);
        return;
    }

    game_init(state);

    if (r == SAVE_ERR_NOFILE)
        asm_event_push(state, 0x42, EVT_MILESTONE, 0xFF);
    else {
        fprintf(stderr, "[save] Load failed: %s\n", save_result_str(r));
        asm_event_push(state, 0x4A, EVT_NEGATIVE, 0xFF);
    }
}
