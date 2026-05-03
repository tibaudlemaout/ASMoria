#include "ui_breach.h"
#include "../render/renderer.h"
#include "../../include/asmoria.h"
#include <stdio.h>
#include <string.h>

#define _DIVIDER_COL    90
#define _UI_COL_MARGIN   2

int ui_breach_selected_guard = 0;

/* Flash state for HP damage visual */
static uint64_t last_enemy_hp  = 0;
static uint64_t last_guard_hp[RAID_MAX_GUARDS] = {0};
static uint32_t flash_ticks    = 0;   /* counts down after damage */

void ui_breach_select(int delta) {
    ui_breach_selected_guard += delta;
    if (ui_breach_selected_guard < 0) ui_breach_selected_guard = 0;
}

/* =========================================================
 * ASCII enemy sprites — one per threat tier
 * Each sprite is 5 lines tall, 20 chars wide
 * ========================================================= */
static const char *sprite_goblin[] = {
    "    .---.     ",
    "   ( o o )    ",
    "    > ^ <     ",
    "   /|   |\\   ",
    "  (_|   |_)   ",
};
static const char *sprite_troll[] = {
    "  .--------.  ",
    " ( O     O )  ",
    "  |  ___  |   ",
    "  \\ (___) /  ",
    "  /|     |\\ ",
};
static const char *sprite_demon[] = {
    " /\\  . .  /\\ ",
    "(  \\ o o /  )",
    " \\  > ^ <  / ",
    "  | ----- |   ",
    " /\\_______/\\ ",
};

static const char **get_sprite(int threat) {
    if (threat <= 2) return sprite_goblin;
    if (threat <= 4) return sprite_troll;
    return sprite_demon;
}

static const char *get_enemy_name(int threat) {
    if (threat == 1) return "Cave Goblin";
    if (threat == 2) return "Goblin Raider";
    if (threat == 3) return "Stone Troll";
    if (threat == 4) return "War Troll";
    return "Demon of the Deep";
}

/* =========================================================
 * HP bar — 12 chars wide, colour shifts red as HP drops
 * ========================================================= */
static void make_hp_bar(char *out, int hp, int hp_max) {
    int filled = (hp_max > 0 && hp > 0) ? (hp * 12 / hp_max) : 0;
    if (filled > 12) filled = 12;
    out[0] = '[';
    for (int i = 0; i < 12; i++)
        out[i+1] = (i < filled) ? '#' : '.';
    out[13] = ']';
    out[14] = '\0';
}

static uint32_t hp_color(int hp, int hp_max) {
    if (hp_max <= 0) return COL_DIM;
    int pct = hp * 100 / hp_max;
    if (pct > 60) return COL_FOOD;     /* green  */
    if (pct > 30) return COL_GOLD;     /* yellow */
    return 0xFF4444FF;                  /* red    */
}

/* =========================================================
 * ui_draw_breach
 * ========================================================= */
void ui_draw_breach(Renderer *r, const GameState *state) {
    const Raid *raid = &state->raid;
    char buf[128];
    char hp_bar[16];
    int row = 0;

    /* Detect damage flashes */
    if (raid->active == RAID_COMBAT) {
        if ((int32_t)last_enemy_hp > raid->enemy_hp)
            flash_ticks = 6;
        last_enemy_hp = raid->enemy_hp;
    }

    /* Header */
    uint32_t header_col =
        raid->active == RAID_COMBAT  ? 0xFF4444FF :
        raid->active == RAID_WARNING ? COL_GOLD   : COL_DIM;

    const char *phase_str =
        raid->active == RAID_WARNING ? "  !! WARNING — ASSIGN GUARDS !!" :
        raid->active == RAID_COMBAT  ? "  !! COMBAT IN PROGRESS !!"      :
        raid->active == RAID_RESULT  ? "  !! RAID RESOLVED !!"           :
                                       "";

    renderer_draw_text_grid(r, _UI_COL_MARGIN, row, header_col, "[ THE BREACH ]");
    if (phase_str[0])
        renderer_draw_text_grid(r, _UI_COL_MARGIN + 15, row, header_col, phase_str);
    renderer_draw_hline_partial(r, 1, 0, _DIVIDER_COL, COL_DIM);
    row = 2;

    /* -------------------------------------------------------
     * No raid
     * ----------------------------------------------------- */
    if (raid->active == RAID_NONE) {
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM,
            "No raid in progress. The deep is calm... for now.");
        row++;
        if (raid->next_raid_tick > 0) {
            uint64_t ticks_left = raid->next_raid_tick > state->tick
                ? raid->next_raid_tick - state->tick : 0;
            snprintf(buf, sizeof(buf),
                     "Next warning in ~%llu ticks (~%llu seconds)",
                     (unsigned long long)ticks_left,
                     (unsigned long long)(ticks_left / 2));
            renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM, buf);
        }
        renderer_draw_hline_partial(r, 41, 0, _DIVIDER_COL, COL_DIM);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 42, COL_DIM, "  [B] Close");
        return;
    }

    /* -------------------------------------------------------
     * Warning phase
     * ----------------------------------------------------- */
    if (raid->active == RAID_WARNING) {
        /* Threat level */
        int show_threat1 = raid->threat;
        if (show_threat1 < 1) show_threat1 = 1;
        if (show_threat1 > 5) show_threat1 = 5;
        snprintf(buf, sizeof(buf), "Threat Level %d — %s",
                 show_threat1, get_enemy_name(show_threat1));
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_ACCENT, buf);
        row += 2;
        /* Sprite (dimmed during warning) */
        const char **sprite = get_sprite(raid->threat);
        for (int i = 0; i < 5; i++) {
            renderer_draw_text_grid(r, _UI_COL_MARGIN + 4, row + i,
                                    COL_DIM, sprite[i]);
        }
        row += 6;

        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_GOLD,
            "Scouts report movement in the deep!");
        row++;
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_GOLD,
            "Assign guards before the raid arrives.");
        row += 2;

        int gc = 0;
        for (int i = 0; i < MAX_DWARVES; i++)
            if (state->dwarves[i].alive && state->dwarves[i].job == JOB_GUARD)
                gc++;

        snprintf(buf, sizeof(buf), "Guards ready: %d", gc);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row,
                                gc > 0 ? COL_FOOD : 0xFF4444FF, buf);

        renderer_draw_hline_partial(r, 41, 0, _DIVIDER_COL, COL_DIM);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 42, COL_DIM,
            "  [B] Close    Assign guards with [G] key");
        return;
    }

    /* -------------------------------------------------------
     * Combat phase
     * ----------------------------------------------------- */
    if (raid->active == RAID_COMBAT) {
        /* Threat level */
        int show_threat2 = raid->threat;
        if (show_threat2 < 1) show_threat2 = 1;
        if (show_threat2 > 5) show_threat2 = 5;
        snprintf(buf, sizeof(buf), "Threat Level %d — %s",
                 show_threat2, get_enemy_name(show_threat2));
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_ACCENT, buf);
        row += 2;
        /* Enemy sprite — flash white on damage */
        const char **sprite = get_sprite(raid->threat);
        uint32_t sprite_col = (flash_ticks > 0) ? 0xFFFFFFFF : 0xFF4444FF;
        if (flash_ticks > 0) flash_ticks--;

        for (int i = 0; i < 5; i++) {
            renderer_draw_text_grid(r, _UI_COL_MARGIN + 4, row + i,
                                    sprite_col, sprite[i]);
        }
        row += 6;

        /* Enemy HP bar */
        make_hp_bar(hp_bar, raid->enemy_hp, raid->enemy_hp_max);
        uint32_t ehp_col = hp_color(raid->enemy_hp, raid->enemy_hp_max);
        snprintf(buf, sizeof(buf), "%s  HP: %d / %d   ATK: %d",
                 hp_bar, raid->enemy_hp, raid->enemy_hp_max, raid->enemy_atk);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, ehp_col, buf);
        row += 2;

        /* Divider */
        renderer_draw_hline_partial(r, row, 0, _DIVIDER_COL, COL_DIM);
        row++;

        /* Guards */
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_FG, "YOUR GUARDS");
        row++;

        if (ui_breach_selected_guard >= raid->guard_count && raid->guard_count > 0)
            ui_breach_selected_guard = raid->guard_count - 1;

        for (int i = 0; i < raid->guard_count; i++) {
            uint8_t didx = raid->guard_idx[i];
            const Dwarf *d = &state->dwarves[didx];
            const char *name = asm_get_dwarf_name(d->name_idx);
            int hp     = raid->guard_hp[i];
            int hp_max = raid->guard_hp_max[i];
            int ko     = (hp <= 0);
            int sel    = (i == ui_breach_selected_guard);

            /* Flash guard bar on damage */
            uint32_t gflash_col = 0;
            if ((int32_t)last_guard_hp[i] > hp) gflash_col = 0xFFFFFFFF;
            last_guard_hp[i] = hp;

            make_hp_bar(hp_bar, ko ? 0 : hp, hp_max);
            uint32_t gcol = gflash_col ? gflash_col
                          : ko         ? COL_DIM
                          : sel        ? COL_ACCENT
                          :              hp_color(hp, hp_max);

            int guard_atk = 5 + d->job_level[JOB_GUARD] * 3;
            snprintf(buf, sizeof(buf), "%s %-12s Lv%d  %s  %d/%d  ATK:%d%s",
                     sel ? ">" : " ",
                     name,
                     d->job_level[JOB_GUARD],
                     hp_bar,
                     ko ? 0 : hp, hp_max,
                     guard_atk,
                     ko ? "  [KO]" : "");

            renderer_draw_text_grid(r, _UI_COL_MARGIN, row, gcol, buf);
            row++;
        }

        row++;
        renderer_draw_hline_partial(r, row, 0, _DIVIDER_COL, COL_DIM);
        row++;
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM,
            "  [UP/DN] Select   [E] Feed guard   [R] Retreat   [B] Close");
        return;
    }

    /* -------------------------------------------------------
     * Result phase
     * ----------------------------------------------------- */
    if (raid->active == RAID_RESULT) {
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM,
            "The raid has concluded.");
        renderer_draw_hline_partial(r, 41, 0, _DIVIDER_COL, COL_DIM);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 42, COL_DIM,
            "  [B] Close");
    }
}
