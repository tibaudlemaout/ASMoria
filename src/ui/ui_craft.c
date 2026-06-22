#include "ui_craft.h"
#include "../render/renderer.h"
#include "../../include/asmoria.h"
#include <stdio.h>
#include <string.h>

#define _DIVIDER_COL    90
#define _UI_COL_MARGIN   2

int ui_craft_cursor = 0;

void ui_craft_move(int delta) {
    ui_craft_cursor += delta;
    if (ui_craft_cursor < 0)              ui_craft_cursor = RECIPE_COUNT - 1;
    if (ui_craft_cursor >= RECIPE_COUNT)  ui_craft_cursor = 0;
}

int ui_craft_selected(void) { return ui_craft_cursor; }

/* Recipe display info */
typedef struct {
    const char *name;
    int         tier;
    int         dwarves_per_unit;
    int         timer_ticks;
    const char *input;
    const char *output;
    int         depth_req;    /* minimum depth to unlock */
} RecipeInfo;

static const RecipeInfo recipes[RECIPE_COUNT] = {
    { "Iron Bars I",   1, 1, 5,  "2 Iron Ore",               "1 Iron Bar",  2 },
    { "Ale I",         1, 1, 3,  "1 Food + 1 Wood",           "1 Ale",       1 },
    { "Iron Bars II",  2, 1, 5,  "3 Iron Ore",               "2 Iron Bars", 2 },
    { "Weapons I",     2, 2, 10, "3 Iron Bars",              "1 Weapon",    3 },
    { "Armour I",      2, 2, 15, "4 Iron Bars",              "1 Armour",    3 },
    { "Tools I",       2, 2, 10, "2 Iron Bars + 1 Gem",      "1 Tool",      3 },
    { "Wall",          2, 1,  8, "5 Stone + 2 Iron Bars",     "1 Wall",      2 },
    { "Spike Trap",    2, 1, 12, "3 Iron Bars + 1 Tool",      "1 Spike Trap", 2 },
    { "Slow Trap",     2, 1, 10, "2 Iron Bars + 1 Gem",       "1 Slow Trap",  2 },
};

/* Count total craftsdwarves and free ones */
static void count_craftsdwarves(const GameState *state, int *total, int *free) {
    *total = 0;
    int assigned = 0;
    for (int i = 0; i < MAX_DWARVES; i++)
        if (state->dwarves[i].alive && state->dwarves[i].job == JOB_CRAFTSDWARF)
            (*total)++;
    for (int i = 0; i < RECIPE_COUNT; i++)
        assigned += state->craft[i].assigned;
    *free = *total - assigned;
    if (*free < 0) *free = 0;
}

void ui_draw_craft(Renderer *r, const GameState *state) {
    char buf[128];
    int row = 0;

    renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_ACCENT,
        "[ WORKSHOP ]   UP/DN navigate   +/- assign dwarves   C close");
    renderer_draw_hline_partial(r, 1, 0, _DIVIDER_COL, COL_DIM);

    /* Craftsdwarf summary */
    int total_craft, free_craft;
    count_craftsdwarves(state, &total_craft, &free_craft);

    int ws_lv = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_WORKSHOP);

    snprintf(buf, sizeof(buf),
             "  Workshop: Lv%d    Craftsdwarves: %d total  %d free  "
             "[C] assign job",
             ws_lv, total_craft, free_craft);
    renderer_draw_text_grid(r, _UI_COL_MARGIN, 2, COL_FG, buf);

    /* Iron Bars / Ale / Weapons / Armour count */
    snprintf(buf, sizeof(buf),
             "  Bars:%-5lld  Ale:%-5lld  Wpn:%-4lld  Arm:%-4lld  Tol:%-4lld  Depth:%u",
             (long long)state->resources.iron_bars,
             (long long)state->resources.ale,
             (long long)state->resources.weapons,
             (long long)state->resources.armour,
             (long long)state->resources.tools,
             state->depth);
    renderer_draw_text_grid(r, _UI_COL_MARGIN, 3, COL_DIM, buf);
    renderer_draw_hline_partial(r, 4, 0, _DIVIDER_COL, COL_DIM);

    if (ws_lv < 1) {
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 5, COL_DIM,
            "  Build the Workshop upgrade to unlock crafting.");
        return;
    }

    row = 5;
    int last_tier = -1;

    for (int i = 0; i < RECIPE_COUNT; i++) {
        const RecipeInfo   *ri = &recipes[i];
        const CraftSlot    *cs = &state->craft[i];
        int sel     = (i == ui_craft_cursor);
        int locked  = (state->depth < ri->depth_req);

        /* Tier header */
        if (ri->tier != last_tier) {
            last_tier = ri->tier;
            snprintf(buf, sizeof(buf), "--- TIER %d ---", ri->tier);
            renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM, buf);
            row++;
        }

        /* Output count = assigned / dwarves_per_unit */
        int output_count = ri->dwarves_per_unit > 0
                         ? cs->assigned / ri->dwarves_per_unit : 0;

        /* Timer bar */
        char timer_bar[15] = "[............]";
        if (cs->active && ri->timer_ticks > 0) {
            int elapsed = ri->timer_ticks - cs->timer;
            int filled  = elapsed * 12 / ri->timer_ticks;
            if (filled < 0) filled = 0;
            if (filled > 12) filled = 12;
            timer_bar[0] = '[';
            for (int t = 0; t < 12; t++)
                timer_bar[t+1] = (t < filled) ? '#' : '.';
            timer_bar[13] = ']';
            timer_bar[14] = '\0';
        }

        /* Recipe name line */
        if (locked) {
            snprintf(buf, sizeof(buf), "%s [LOCKED] %-18s  requires depth %d",
                     sel ? ">" : " ", ri->name, ri->depth_req);
            renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM, buf);
        } else {
            int insufficient = cs->assigned > 0 && output_count == 0;
            snprintf(buf, sizeof(buf), "%s %-18s  Assigned: %d/%d  Output: %d/%d ticks%s",
                     sel ? ">" : " ",
                     ri->name,
                     cs->assigned,
                     ri->dwarves_per_unit,
                     output_count,
                     ri->timer_ticks,
                     insufficient ? " [need more dwarves]" : "");
            uint32_t col = insufficient ? COL_GOLD
                         : sel          ? COL_ACCENT
                         : cs->active   ? COL_FOOD
                         :                COL_FG;
            renderer_draw_text_grid(r, _UI_COL_MARGIN, row, col, buf);
        }
        row++;

        /* Input / output / timer line */
        if (!locked) {
            snprintf(buf, sizeof(buf),
                     "    In: %-24s  Out: %-12s  %s%s",
                     ri->input, ri->output,
                     cs->active ? timer_bar : "[          ]",
                     cs->active ? "" : " (idle)");
            renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM, buf);
        }
        row++;

        /* Dwarf assignment hint for selected */
        if (sel && !locked) {
            snprintf(buf, sizeof(buf),
                     "    [+] Assign dwarf (%d free)   [-] Remove dwarf   "
                     "needs %d dwarf/unit",
                     free_craft, ri->dwarves_per_unit);
            renderer_draw_text_grid(r, _UI_COL_MARGIN, row,
                                    free_craft > 0 ? COL_GOLD : COL_DIM, buf);
        }
        row++;

        if (row >= 40) break;
    }

    renderer_draw_hline_partial(r, 41, 0, _DIVIDER_COL, COL_DIM);
    renderer_draw_text_grid(r, _UI_COL_MARGIN, 42, COL_DIM,
        "  [UP/DN] Navigate   [+] Assign dwarf   [-] Remove   [C] Close");
}
