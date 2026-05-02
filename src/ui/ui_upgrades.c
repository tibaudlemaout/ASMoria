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
    int         cost_gold;
    int         cost_stone;
    int         cost_mana;   /* 0 = no mana cost */
} UpgradeInfo;

static const UpgradeInfo upgrades[UPGR_COUNT] = {
    /* Tools */
    { "Pick Quality",  "Sharper picks for your miners.",
      "+1 stone & gold per miner/level",
      "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS, 0 },

    { "Saw Quality",   "Better saws for your lumberers.",
      "+1 wood per lumberer/level",
      "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS, 0 },

    { "Irrigation",    "Channels and ditches for the farms.",
      "+1 food per farmer/level",
      "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS, 0 },

    /* Workforce */
    { "Barracks",      "More bunks, more dwarves.",
      "+16 dwarf slots per level (base: 16)",
      "WORKFORCE", UPGR_MAX_WORKFORCE,
      UPGR_COST_GOLD_WORK, UPGR_COST_STONE_WORK, 0 },

    { "Recruiters",    "Word spreads to distant holds.",
      "-10 gold & -5 food hire cost/level",
      "WORKFORCE", UPGR_MAX_WORKFORCE,
      UPGR_COST_GOLD_WORK, UPGR_COST_STONE_WORK, 0 },

    /* Infrastructure */
    { "Watch Tower",   "A tower to watch the dark.",
      "Lv1: Unlocks Guard | Lv2: -25% neg event damage | Lv3: -50%",
      "INFRASTRUCTURE", UPGR_MAX_WATCHTOWER,
      UPGR_COST_GOLD_WATCH, UPGR_COST_STONE_WATCH, 0 },

    { "Rune Halls",    "Ancient halls of learning.",
      "Lv1: Unlocks Scholar | Lv2: +mana/scholar | Lv3: +XP all | Lv4: +2mana/scholar | Lv5: Research",
      "INFRASTRUCTURE", UPGR_MAX_RUNEHALLS,
      UPGR_COST_GOLD_RUNE, UPGR_COST_STONE_RUNE, UPGR_COST_MANA_RUNE },

    { "Mana Well",     "A well tapping arcane depths.",
      "+2 mana/tick per level (passive)",
      "INFRASTRUCTURE", UPGR_MAX_MANAWELL,
      UPGR_COST_GOLD_MANA, UPGR_COST_STONE_MANA, 0 },
};

void ui_upgr_move(int delta) {
    ui_upgr_cursor += delta;
    if (ui_upgr_cursor < 0)           ui_upgr_cursor = UPGR_COUNT - 1;
    if (ui_upgr_cursor >= UPGR_COUNT) ui_upgr_cursor = 0;
}

int ui_upgr_selected(void) { return ui_upgr_cursor; }

void ui_draw_upgrades(Renderer *r, const GameState *state) {
    char buf[128];
    int row = 0;

    /* Live stats */
    int bar_lv    = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_BARRACKS);
    int rec_lv    = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_RECRUITERS);
    int wt_lv     = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_WATCH_TOWER);
    int rh_lv     = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_RUNE_HALLS);
    int mw_lv     = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_MANA_WELL);
    int dwarf_cap = DWARF_CAP_BASE + bar_lv * DWARF_CAP_PER_LEVEL;
    int hire_gold = HIRE_GOLD_BASE - rec_lv * HIRE_GOLD_DISCOUNT;
    int hire_food = HIRE_FOOD_BASE - rec_lv * HIRE_FOOD_DISCOUNT;
    if (hire_gold < 10) hire_gold = 10;
    if (hire_food < 5)  hire_food = 5;

    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_ACCENT,
        "[ UPGRADES ]    UP/DN navigate    ENTER buy    U close");
    renderer_draw_hline_partial(r, 1, 0, DIVIDER_COL, COL_DIM);

    snprintf(buf, sizeof(buf),
             "  Cap: %d  Hire: %dg/%df  Guards: %s  Scholars: %s  Mana/tick: %d",
             dwarf_cap, hire_gold, hire_food,
             wt_lv >= 1 ? "unlocked" : "locked",
             rh_lv >= 1 ? "unlocked" : "locked",
             mw_lv * 2);
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
        for (int l = 0; l < u->max_level && l < 8; l++)
            bar[l + 1] = (l < level) ? '#' : '.';
        bar[u->max_level + 1] = ']';
        bar[u->max_level + 2] = '\0';

        int cost_gold  = u->cost_gold  * next;
        int cost_stone = u->cost_stone * next;
        int cost_mana  = (u->cost_mana > 0 && next >= 2) ? u->cost_mana * next : 0;

        int can_afford = !maxed
            && state->resources.gold  >= cost_gold
            && state->resources.stone >= cost_stone
            && state->resources.mana  >= cost_mana;

        /* Name row */
        snprintf(buf, sizeof(buf), "%s %-18s %s  Lv %d/%d",
                 sel ? ">" : " ", u->name, bar, level, u->max_level);
        renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                sel ? COL_ACCENT : COL_FG, buf);
        row++;

        /* Effect */
        snprintf(buf, sizeof(buf), "    %s", u->effect);
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, buf);
        row++;

        /* Cost / status */
        if (maxed) {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                    COL_FOOD, "    [MAXED OUT]");
        } else if (cost_mana > 0) {
            snprintf(buf, sizeof(buf),
                     "    Next: %d gold  %d stone  %d mana",
                     cost_gold, cost_stone, cost_mana);
            renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                    can_afford ? COL_GOLD : COL_DIM, buf);
        } else {
            snprintf(buf, sizeof(buf),
                     "    Next: %d gold  %d stone",
                     cost_gold, cost_stone);
            renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                    can_afford ? COL_GOLD : COL_DIM, buf);
        }
        row++;
        row++; /* spacer */

        if (row > 40) break;
    }

    renderer_draw_hline_partial(r, 41, 0, DIVIDER_COL, COL_DIM);
    renderer_draw_text_grid(r, UI_COL_MARGIN, 42, COL_DIM,
        "  [U] Back    [UP/DN] Navigate    [ENTER] Buy");
}
