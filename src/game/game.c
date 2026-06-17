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

    /* Craft slots */
    for (int i = 0; i < RECIPE_COUNT; i++) {
        state->craft[i].assigned = 0;
        state->craft[i].active   = 0;
        state->craft[i].timer    = 0;
    }

    /* New resources (zeroed by memset but explicit for clarity) */
    state->resources.iron_ore  = 0;
    state->resources.gems      = 0;
    state->resources.relics    = 0;
    state->resources.crystals  = 0;
    state->resources.iron_bars = 0;
    state->resources.ale       = 0;
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
    state->raid.raids_completed  = 0;

    state->prestige.honor           = 0;
    state->prestige.total_honor     = 0;
    state->prestige.total_prestiges = 0;
    state->prestige.nodes           = 0;
    state->prestige.total_resources = 0;

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

static void sanitise_craft(GameState *state) {
    for (int i = 0; i < RECIPE_COUNT; i++) {
        /* Clear any garbage assigned counts or stuck timers */
        if (state->craft[i].assigned > 64)
            state->craft[i].assigned = 0;
        if (!state->craft[i].assigned) {
            state->craft[i].active = 0;
            state->craft[i].timer  = 0;
        }
    }
    /* Clamp new resources to sane values */
    if (state->resources.iron_bars < 0) state->resources.iron_bars = 0;
    if (state->resources.ale       < 0) state->resources.ale       = 0;
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
        sanitise_craft(state);
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
