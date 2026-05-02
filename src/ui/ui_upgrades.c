#include "ui_upgrades.h"
#include "ui.h"
#include <stdio.h>

int ui_upgr_cursor = 0;

typedef struct {
    const char *name;
    const char *category;
    int         max_level;
    int         cost_gold;
    int         cost_stone;
    int         cost_mana;
    const char *level_effects[5]; /* what each level (1..max) unlocks */
} UpgradeInfo;

static const UpgradeInfo upgrades[UPGR_COUNT] = {
    { "Pick Quality", "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS, 0,
      { "+1 stone & gold per miner", "+2 stone & gold per miner", "+3 stone & gold per miner" } },

    { "Saw Quality", "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS, 0,
      { "+1 wood per lumberer", "+2 wood per lumberer", "+3 wood per lumberer" } },

    { "Irrigation", "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS, 0,
      { "+1 food per farmer", "+2 food per farmer", "+3 food per farmer" } },

    { "Barracks", "WORKFORCE", UPGR_MAX_WORKFORCE,
      UPGR_COST_GOLD_WORK, UPGR_COST_STONE_WORK, 0,
      { "Dwarf cap: 32", "Dwarf cap: 48", "Dwarf cap: 64" } },

    { "Recruiters", "WORKFORCE", UPGR_MAX_WORKFORCE,
      UPGR_COST_GOLD_WORK, UPGR_COST_STONE_WORK, 0,
      { "Hire cost: 40g/15f", "Hire cost: 30g/10f", "Hire cost: 10g/5f" } },

    { "Watch Tower", "INFRASTRUCTURE", UPGR_MAX_WATCHTOWER,
      UPGR_COST_GOLD_WATCH, UPGR_COST_STONE_WATCH, 0,
      { "Unlocks Guard job", "Guards reduce neg event damage by 25%",
        "Guards reduce neg event damage by 50%" } },

    { "Rune Halls", "INFRASTRUCTURE", UPGR_MAX_RUNEHALLS,
      UPGR_COST_GOLD_RUNE, UPGR_COST_STONE_RUNE, UPGR_COST_MANA_RUNE,
      { "Unlocks Scholar job", "Scholars generate +1 mana/tick",
        "All working dwarves gain +1 XP/tick", "Scholars generate +2 mana/tick",
        "Unlocks Research" } },

    { "Mana Well", "INFRASTRUCTURE", UPGR_MAX_MANAWELL,
      UPGR_COST_GOLD_MANA, UPGR_COST_STONE_MANA, 0,
      { "Passive +2 mana/tick", "Passive +4 mana/tick", "Passive +6 mana/tick" } },
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

        /* Name + level bar */
        snprintf(buf, sizeof(buf), "%s %-18s %s  Lv %d/%d",
                 sel ? ">" : " ", u->name, bar, level, u->max_level);
        renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                sel ? COL_ACCENT : COL_FG, buf);
        row++;

        /* Next level effect — only show what the NEXT level does */
        if (maxed) {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                    COL_FOOD, "    [MAXED OUT]");
        } else {
            snprintf(buf, sizeof(buf), "    Next: %s",
                     u->level_effects[level]);   /* level = current, so index = next-1 */
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, buf);
        }
        row++;

        /* Cost */
        if (!maxed) {
            if (cost_mana > 0)
                snprintf(buf, sizeof(buf), "    Cost: %d gold  %d stone  %d mana",
                         cost_gold, cost_stone, cost_mana);
            else
                snprintf(buf, sizeof(buf), "    Cost: %d gold  %d stone",
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
