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
    state->pending.code      = 0xFF;
    state->raid.active         = RAID_NONE;
    state->raid.threat         = 0;
    state->raid.guard_count    = 0;
    state->raid.enemy_hp       = 0;
    state->raid.enemy_hp_max   = 0;
    state->raid.enemy_atk      = 0;
    state->raid.next_raid_tick = 0; /* breach.asm sets this on first tick */
    state->raid.combat_start_tick = 0;
    state->raid.last_combat_tick  = 0;
    state->raid.reward_gold    = 0;
    state->raid.raids_completed = 0;

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

static void sanitise_raid(GameState *state) {
    Raid *raid = &state->raid;
    /* If next_raid_tick looks garbage (> current_tick + 10000), reset it */
    if (raid->active == RAID_NONE || raid->active > RAID_RESULT) {
        raid->active      = RAID_NONE;
        raid->threat      = 0;
        raid->guard_count = 0;
        /* Reset next_raid_tick to a sane value */
        if (raid->next_raid_tick == 0 ||
            raid->next_raid_tick > state->tick + 10000ULL ||
            raid->next_raid_tick < state->tick) {
            raid->next_raid_tick = 0; /* breach.asm will init on first tick */
        }
        raid->raids_completed = (raid->raids_completed < 100)
                                ? raid->raids_completed : 0;
    }
}

void game_load_or_init(GameState *state) {
    SaveResult r = load_game(state);

    if (r == SAVE_OK) {
        sanitise_raid(state);
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
