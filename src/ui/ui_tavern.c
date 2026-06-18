#include "ui_tavern.h"
#include "../render/renderer.h"
#include "../../include/asmoria.h"
#include <stdio.h>
#include <string.h>

#define _DIVIDER_COL    90
#define _UI_COL_MARGIN   2

int ui_tavern_cursor = 0;

void ui_tavern_move(int delta) {
    ui_tavern_cursor += delta;
    if (ui_tavern_cursor < 0)                    ui_tavern_cursor = BUFF_COUNT - 2;
    if (ui_tavern_cursor >= BUFF_COUNT - 1)      ui_tavern_cursor = 0;
}

int ui_tavern_selected(void) { return ui_tavern_cursor + 1; } /* +1 skips BUFF_NONE */

typedef struct {
    const char *name;
    const char *desc;
    int         ale_cost;
    int         duration;
    const char *category;
} BuffInfo;

static const BuffInfo buffs[BUFF_COUNT] = {
    { "---",                "---",                                               0,   0, "" },
    { "Feast",              "+20% yield, +5 fatigue/tick",                     200, 100, "GENERAL" },
    { "Drinking Contest",   "-10% yield, fatigue generation halved",           150, 100, "GENERAL" },
    { "Toast",              "Restore morale to 80, +1 XP/tick all",           100, 100, "GENERAL" },
    { "Long Rest",          "Fatigue recovery 2x, morale decay halved",         80,  80, "GENERAL" },
    { "Miner's Courage",    "Miners ignore depth fatigue penalty",              80,  60, "JOB" },
    { "Lumberjack's Song",  "+2 wood/tick per lumberer",                        70,  60, "JOB" },
    { "Harvest Festival",   "+2 food/tick per farmer",                          70,  60, "JOB" },
    { "Warrior's Draft",    "+8 ATK, +15 HP for guards",                       100,  80, "JOB" },
    { "Scholar's Clarity",  "Scholars generate +50% mana",                      90,  80, "JOB" },
    { "Craftsdwarf's Focus","Craft timers tick 2x faster",                      80,  60, "JOB" },
};

void ui_draw_tavern(Renderer *r, const GameState *state) {
    char buf[128];
    int row = 0;

    renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_ACCENT,
        "[ THE TAVERN ]   UP/DN navigate   ENTER activate   T close");
    renderer_draw_hline_partial(r, 1, 0, _DIVIDER_COL, COL_DIM);

    uint32_t lv = state->tavern_level;
    int max_slots = (int)lv < MAX_TAVERN_BUFFS ? (int)lv : MAX_TAVERN_BUFFS;

    snprintf(buf, sizeof(buf),
             "  Tavern Level: %u/%u    Ale: %-6lld    Active buffs: %d/%d",
             lv, TAVERN_MAX_LEVEL,
             (long long)state->resources.ale,
             0, max_slots);

    /* Count active buffs */
    int active_count = 0;
    for (int i = 0; i < MAX_TAVERN_BUFFS; i++)
        if (state->tavern_buffs[i].active) active_count++;

    snprintf(buf, sizeof(buf),
             "  Tavern Level: %u/%u    Ale: %-6lld    Active buffs: %d/%d",
             lv, TAVERN_MAX_LEVEL,
             (long long)state->resources.ale,
             active_count, max_slots);
    renderer_draw_text_grid(r, _UI_COL_MARGIN, 2, COL_FG, buf);

    /* Active buff slots */
    renderer_draw_hline_partial(r, 3, 0, _DIVIDER_COL, COL_DIM);
    renderer_draw_text_grid(r, _UI_COL_MARGIN, 4, COL_DIM, "  Active buffs:");
    for (int i = 0; i < MAX_TAVERN_BUFFS; i++) {
        const TavernBuff *tb = &state->tavern_buffs[i];
        if (i >= max_slots) {
            snprintf(buf, sizeof(buf), "    Slot %d: [LOCKED]", i + 1);
            renderer_draw_text_grid(r, _UI_COL_MARGIN, 5 + i, COL_DIM, buf);
        } else if (!tb->active) {
            snprintf(buf, sizeof(buf), "    Slot %d: [empty]", i + 1);
            renderer_draw_text_grid(r, _UI_COL_MARGIN, 5 + i, COL_DIM, buf);
        } else {
            const BuffInfo *bi = (tb->buff_id < BUFF_COUNT) ? &buffs[tb->buff_id] : &buffs[0];
            char timerbar[14];
            int dur = bi->duration > 0 ? bi->duration : 1;
            int elapsed = dur - tb->timer;
            int filled = elapsed * 12 / dur;
            if (filled < 0) filled = 0;
            if (filled > 12) filled = 12;
            timerbar[0] = '[';
            for (int t = 0; t < 12; t++) timerbar[t+1] = (t < filled) ? '#' : '.';
            timerbar[13] = ']'; timerbar[13] = '\0';
            snprintf(buf, sizeof(buf), "    Slot %d: %-22s  %s  %d ticks left",
                     i + 1, bi->name, timerbar, tb->timer);
            renderer_draw_text_grid(r, _UI_COL_MARGIN, 5 + i, COL_FOOD, buf);
        }
    }

    renderer_draw_hline_partial(r, 8, 0, _DIVIDER_COL, COL_DIM);

    if (lv == 0) {
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 9, COL_DIM,
            "  Build the Tavern upgrade to unlock buffs.");
        return;
    }

    /* Buff list */
    row = 9;
    const char *last_cat = NULL;

    for (int i = 1; i < BUFF_COUNT; i++) {
        const BuffInfo *bi = &buffs[i];
        int sel = (i - 1 == ui_tavern_cursor);

        /* Category header */
        if (last_cat != bi->category) {
            last_cat = bi->category;
            snprintf(buf, sizeof(buf), "  --- %s BUFFS ---", bi->category);
            renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM, buf);
            row++;
        }

        /* Check if already active */
        int is_active = (int)asm_tavern_buff_active((GameState *)state, (uint8_t)i);
        int can_afford = state->resources.ale >= bi->ale_cost;
        int slot_free = active_count < max_slots;

        snprintf(buf, sizeof(buf), "%s %-22s  %3d ale  %3d ticks  %s",
                 sel ? ">" : " ",
                 bi->name,
                 bi->ale_cost,
                 bi->duration,
                 bi->desc);

        uint32_t col = is_active  ? COL_FOOD
                     : !can_afford ? COL_DIM
                     : !slot_free  ? COL_DIM
                     : sel         ? COL_ACCENT
                     :               COL_FG;

        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, col, buf);
        if (is_active)
            renderer_draw_text_grid(r, _UI_COL_MARGIN + 60, row, COL_FOOD, "[ACTIVE]");
        else if (!can_afford)
            renderer_draw_text_grid(r, _UI_COL_MARGIN + 60, row, COL_DIM, "[need ale]");
        else if (!slot_free)
            renderer_draw_text_grid(r, _UI_COL_MARGIN + 60, row, COL_DIM, "[no slot]");

        row++;
        if (row >= 41) break;
    }

    renderer_draw_hline_partial(r, 41, 0, _DIVIDER_COL, COL_DIM);
    renderer_draw_text_grid(r, _UI_COL_MARGIN, 42, COL_DIM,
        "  [UP/DN] Navigate   [ENTER] Activate buff   [T] Close");
}
