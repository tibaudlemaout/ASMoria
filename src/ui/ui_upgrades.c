#include "ui_upgrades.h"
#include "ui.h"
#include <stdio.h>

int ui_upgr_cursor = 0;

typedef struct {
    const char *name;
    const char *desc;
    const char *effect;
    const char *category;
    int         max_level;
    int         cost_gold_base;
    int         cost_stone_base;
} UpgradeInfo;

static const UpgradeInfo upgrades[UPGR_COUNT] = {
    { "Pick Quality",  "Sharper picks for your miners.",
      "+1 stone & +0.5 gold per miner/level",
      "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS },

    { "Saw Quality",   "Better saws for your lumberers.",
      "+1 wood per lumberer per level",
      "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS },

    { "Irrigation",    "Channels and ditches for the farms.",
      "+1 food per farmer per level",
      "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS },

    { "Barracks",      "More bunks for more dwarves.",
      "+16 dwarf slots per level (base: 16)",
      "WORKFORCE", UPGR_MAX_WORKFORCE,
      UPGR_COST_GOLD_WORK, UPGR_COST_STONE_WORK },

    { "Recruiters",    "Word spreads to distant holds.",
      "-10 gold & -5 food hire cost per level",
      "WORKFORCE", UPGR_MAX_WORKFORCE,
      UPGR_COST_GOLD_WORK, UPGR_COST_STONE_WORK },
};

void ui_upgr_move(int delta) {
    ui_upgr_cursor += delta;
    if (ui_upgr_cursor < 0)           ui_upgr_cursor = UPGR_COUNT - 1;
    if (ui_upgr_cursor >= UPGR_COUNT) ui_upgr_cursor = 0;
}

int ui_upgr_selected(void) { return ui_upgr_cursor; }

/* =========================================================
 * Draw upgrade panel
 * ========================================================= */
void ui_draw_upgrades(Renderer *r, const GameState *state) {
    char buf[128];
    int row = 0;

    /* Compute live values */
    int bar_lv   = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_BARRACKS);
    int rec_lv   = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_RECRUITERS);
    int dwarf_cap = DWARF_CAP_BASE + bar_lv * DWARF_CAP_PER_LEVEL;
    int hire_gold = HIRE_GOLD_BASE - rec_lv * HIRE_GOLD_DISCOUNT;
    int hire_food = HIRE_FOOD_BASE - rec_lv * HIRE_FOOD_DISCOUNT;
    if (hire_gold < 10) hire_gold = 10;
    if (hire_food < 5)  hire_food = 5;

    /* Header */
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_ACCENT,
        "[ UPGRADES ]    UP/DN navigate    ENTER buy    U close");
    renderer_draw_hline_partial(r, 1, 0, DIVIDER_COL, COL_DIM);

    /* Live stats row */
    snprintf(buf, sizeof(buf),
             "  Dwarf cap: %d    Hire cost: %d gold / %d food",
             dwarf_cap, hire_gold, hire_food);
    renderer_draw_text_grid(r, UI_COL_MARGIN, 2, COL_DIM, buf);
    renderer_draw_hline_partial(r, 3, 0, DIVIDER_COL, COL_DIM);
    row = 4;

    const char *last_cat = NULL;

    for (int i = 0; i < UPGR_COUNT; i++) {
        const UpgradeInfo *u = &upgrades[i];
        int level  = (int)UPGR_LEVEL(state->upgrades.tier1, i);
        int next   = level + 1;
        int maxed  = (level >= u->max_level);
        int sel    = (i == ui_upgr_cursor);

        /* Category separator */
        if (last_cat != u->category) {
            last_cat = u->category;
            snprintf(buf, sizeof(buf), "--- %s ---", u->category);
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, buf);
            row++;
        }

        /* Level bar */
        char bar[10];
        bar[0] = '[';
        for (int l = 0; l < u->max_level; l++)
            bar[l + 1] = (l < level) ? '#' : '.';
        bar[u->max_level + 1] = ']';
        bar[u->max_level + 2] = '\0';

        int cost_gold  = u->cost_gold_base  * next;
        int cost_stone = u->cost_stone_base * next;
        int can_afford = !maxed
            && state->resources.gold  >= cost_gold
            && state->resources.stone >= cost_stone;

        /* Name row */
        snprintf(buf, sizeof(buf), "%s %-18s %s  Lv %d/%d",
                 sel ? ">" : " ", u->name, bar, level, u->max_level);
        renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                sel ? COL_ACCENT : COL_FG, buf);
        row++;

        /* Description */
        snprintf(buf, sizeof(buf), "    %s", u->desc);
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, buf);
        row++;

        /* Effect */
        snprintf(buf, sizeof(buf), "    %s", u->effect);
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, buf);
        row++;

        /* Cost / status */
        if (maxed) {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                    COL_FOOD, "    [MAXED OUT]");
        } else {
            snprintf(buf, sizeof(buf),
                     "    Next level: %d gold   %d stone",
                     cost_gold, cost_stone);
            renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                    can_afford ? COL_GOLD : COL_DIM, buf);
        }
        row++;
        row++; /* spacer */

        if (row > 38) break;
    }

    renderer_draw_hline_partial(r, 39, 0, DIVIDER_COL, COL_DIM);
    renderer_draw_text_grid(r, UI_COL_MARGIN, 40, COL_DIM,
        "  [U] Back    [UP/DN] Navigate    [ENTER] Buy upgrade");
}
