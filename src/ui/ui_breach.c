#include "ui_breach.h"
#include "../render/renderer.h"
#include "../../include/asmoria.h"
#include <stdio.h>
#include <string.h>

#define _DIVIDER_COL    90
#define _UI_COL_MARGIN   2

int ui_breach_selected_guard = 0;

void ui_breach_select(int delta) {
    ui_breach_selected_guard += delta;
    if (ui_breach_selected_guard < 0) ui_breach_selected_guard = 0;
}

static void make_hp_bar(char *out, int hp, int hp_max) {
    int filled = (hp_max > 0) ? (hp * 10 / hp_max) : 0;
    if (filled < 0) filled = 0;
    if (filled > 10) filled = 10;
    out[0] = '[';
    for (int i = 0; i < 10; i++)
        out[i+1] = (i < filled) ? '#' : '.';
    out[11] = ']';
    out[12] = '\0';
}

void ui_draw_breach(Renderer *r, const GameState *state) {
    const Raid *raid = &state->raid;
    char buf[128];
    int row = 0;

    /* Header */
    const char *phase_str =
        raid->active == RAID_WARNING ? "  !! WARNING !!" :
        raid->active == RAID_COMBAT  ? "  !! COMBAT !!"  :
        raid->active == RAID_RESULT  ? "  !! RESOLVED !!" : "  No active raid";

    uint32_t header_col =
        raid->active == RAID_COMBAT  ? 0xFF4444FF :
        raid->active == RAID_WARNING ? COL_GOLD   : COL_DIM;

    renderer_draw_text_grid(r, _UI_COL_MARGIN, row, header_col,
        "[ THE BREACH ]");
    renderer_draw_text_grid(r, _UI_COL_MARGIN + 15, row, header_col, phase_str);
    renderer_draw_hline_partial(r, 1, 0, _DIVIDER_COL, COL_DIM);
    row = 2;

    if (raid->active == RAID_NONE) {
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM,
            "The tunnels are quiet. No raid in progress.");
        row++;
        snprintf(buf, sizeof(buf), "Next raid warning at tick: %llu",
                 (unsigned long long)raid->next_raid_tick);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM, buf);
        renderer_draw_hline_partial(r, 42, 0, _DIVIDER_COL, COL_DIM);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 43, COL_DIM, "  [B] Close");
        return;
    }

    /* Threat level */
    snprintf(buf, sizeof(buf), "Threat Level: %d / 5", raid->threat);
    renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_ACCENT, buf);
    row += 2;

    if (raid->active == RAID_WARNING) {
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_GOLD,
            "Scouts report movement in the deep.");
        row++;
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_GOLD,
            "Assign guards with [G] before the raid arrives!");
        row += 2;
        snprintf(buf, sizeof(buf), "Guards assigned: %d",
                 raid->guard_count);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row,
                                raid->guard_count > 0 ? COL_FOOD : 0xFF4444FF, buf);
        row += 2;
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM,
            "  [B] Close    [G] Assign guard job to selected dwarf");

    } else if (raid->active == RAID_COMBAT) {

        /* Enemy section */
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, 0xFF4444FF, "ENEMY");
        row++;
        char hp_bar[14];
        make_hp_bar(hp_bar, raid->enemy_hp, raid->enemy_hp_max);
        snprintf(buf, sizeof(buf), "HP: %s  %d / %d   ATK: %d/round",
                 hp_bar, raid->enemy_hp, raid->enemy_hp_max, raid->enemy_atk);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, 0xFF4444FF, buf);
        row += 2;

        /* Guards section */
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_FG, "GUARDS");
        row++;

        /* Clamp selected guard */
        if (ui_breach_selected_guard >= raid->guard_count)
            ui_breach_selected_guard = raid->guard_count > 0 ? raid->guard_count - 1 : 0;

        for (int i = 0; i < raid->guard_count; i++) {
            uint8_t didx = raid->guard_idx[i];
            const Dwarf *d = &state->dwarves[didx];
            const char *name = asm_get_dwarf_name(d->name_idx);
            int hp     = raid->guard_hp[i];
            int hp_max = raid->guard_hp_max[i];
            int ko     = (hp <= 0);
            int sel    = (i == ui_breach_selected_guard);

            make_hp_bar(hp_bar, ko ? 0 : hp, hp_max);
            snprintf(buf, sizeof(buf), "%s %-12s Lv%d  %s  %d/%d%s",
                     sel ? ">" : " ",
                     name,
                     d->job_level[JOB_GUARD],
                     hp_bar,
                     ko ? 0 : hp, hp_max,
                     ko ? "  [KO]" : "");

            uint32_t gcol = ko  ? COL_DIM  :
                            sel ? COL_ACCENT : COL_FOOD;
            renderer_draw_text_grid(r, _UI_COL_MARGIN, row, gcol, buf);
            row++;
        }

        row++;
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM,
            "  [UP/DN] Select guard  [E] Feed  [R] Retreat  [B] Close");

    } else if (raid->active == RAID_RESULT) {
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM,
            "The raid has been resolved. Press [B] to dismiss.");
    }

    renderer_draw_hline_partial(r, 42, 0, _DIVIDER_COL, COL_DIM);
}
