#include "ui_upgrades.h"
#include "ui.h"
#include <stdio.h>

int ui_upgr_cursor = 0;
static int ui_upgr_scroll = 0;

typedef struct {
    const char *name;
    const char *category;
    int         max_level;
    int         cost_gold;
    int         cost_stone;
    int         cost_mana;
    int         cost_wood;   /* extra wood cost (Tools, Warehouse) */
    int         cost_food;   /* extra food cost (Granary) */
    const char *level_effects[5]; /* what each level (1..max) unlocks */
} UpgradeInfo;

static const UpgradeInfo upgrades[UPGR_COUNT] = {
    { "Pick Quality", "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS, 0, UPGR_COST_WOOD_TOOLS, 0,
      { "+1 stone & gold per miner", "+2 stone & gold per miner", "+3 stone & gold per miner" } },

    { "Saw Quality", "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS, 0, UPGR_COST_WOOD_TOOLS, 0,
      { "+1 wood per lumberer", "+2 wood per lumberer", "+3 wood per lumberer" } },

    { "Irrigation", "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS, 0, UPGR_COST_WOOD_TOOLS, 0,
      { "+1 food per farmer", "+2 food per farmer", "+3 food per farmer" } },

    { "Barracks", "WORKFORCE", UPGR_MAX_WORKFORCE,
      UPGR_COST_GOLD_WORK, UPGR_COST_STONE_WORK, 0, 0, 0,
      { "Dwarf cap: 24", "Dwarf cap: 32", "Dwarf cap: 40" } },

    { "Recruiters", "WORKFORCE", UPGR_MAX_WORKFORCE,
      UPGR_COST_GOLD_WORK, UPGR_COST_STONE_WORK, 0, 0, 0,
      { "Hire cost -10%", "Hire cost -20%", "Hire cost -30%" } },

    { "Watch Tower", "INFRASTRUCTURE", UPGR_MAX_WATCHTOWER,
      UPGR_COST_GOLD_WATCH, UPGR_COST_STONE_WATCH, 0, 0, 0,
      { "Unlocks Guard job", "Guards reduce neg event damage by 25%",
        "Guards reduce neg event damage by 50%" } },

    { "Rune Halls", "INFRASTRUCTURE", UPGR_MAX_RUNEHALLS,
      UPGR_COST_GOLD_RUNE, UPGR_COST_STONE_RUNE, UPGR_COST_MANA_RUNE, 0, 0,
      { "Unlocks Scholar job", "Scholars generate +1 mana/tick",
        "All working dwarves gain +1 XP/tick", "Scholars generate +2 mana/tick",
        "Unlocks Research" } },

    { "Mana Well", "INFRASTRUCTURE", UPGR_MAX_MANAWELL,
      UPGR_COST_GOLD_MANA, UPGR_COST_STONE_MANA, 0, 0, 0,
      { "Passive +2 mana/tick", "Passive +4 mana/tick", "Passive +6 mana/tick" } },

    { "Vault", "STORAGE", UPGR_MAX_STORAGE,
      UPGR_COST_GOLD_VAULT, UPGR_COST_STONE_VAULT, 0, 0, 0,
      { "Gold cap: 1000", "Gold cap: 1500", "Gold cap: 2000" } },

    { "Warehouse", "STORAGE", UPGR_MAX_STORAGE,
      UPGR_COST_GOLD_WAREHOUSE, UPGR_COST_STONE_WAREHOUSE, 0, UPGR_COST_WOOD_WAREHOUSE, 0,
      { "Stone cap: 1000, Wood cap: 500", "Stone cap: 1500, Wood cap: 800",
        "Stone cap: 2000, Wood cap: 1100" } },

    { "Granary", "STORAGE", UPGR_MAX_STORAGE,
      UPGR_COST_GOLD_GRANARY, UPGR_COST_STONE_GRANARY, 0, 0, UPGR_COST_FOOD_GRANARY,
      { "Food cap: 600", "Food cap: 1000", "Food cap: 1400" } },

    { "Workshop", "CRAFTING", UPGR_MAX_WORKSHOP,
      UPGR_COST_GOLD_WORKSHOP, UPGR_COST_STONE_WORKSHOP, 0, 0, 0,
      { "Unlocks Craftsdwarf job (Iron Bars, Ale)" } },
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

    /* Resources row */
    char seg[32];
    int rcol = UI_COL_MARGIN;
    snprintf(seg, sizeof(seg), "Gold:%-6lld  ", (long long)state->resources.gold);
    renderer_draw_text_grid(r, rcol, 2, COL_GOLD, seg); rcol += (int)strlen(seg);
    snprintf(seg, sizeof(seg), "Stone:%-6lld  ", (long long)state->resources.stone);
    renderer_draw_text_grid(r, rcol, 2, COL_STONE, seg); rcol += (int)strlen(seg);
    snprintf(seg, sizeof(seg), "Wood:%-6lld  ", (long long)state->resources.wood);
    renderer_draw_text_grid(r, rcol, 2, COL_WOOD, seg); rcol += (int)strlen(seg);
    snprintf(seg, sizeof(seg), "Food:%-6lld  ", (long long)state->resources.food);
    renderer_draw_text_grid(r, rcol, 2, COL_FOOD, seg); rcol += (int)strlen(seg);
    snprintf(seg, sizeof(seg), "Mana:%-6lld", (long long)state->resources.mana);
    renderer_draw_text_grid(r, rcol, 2, COL_MANA, seg);
    rcol += (int)strlen(seg);

    /* Iron Bars / Ale — only once Workshop is built */
    int ws_lv = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_WORKSHOP);
    if (ws_lv >= 1) {
        snprintf(seg, sizeof(seg), "  Bars:%-6lld  ", (long long)state->resources.iron_bars);
        renderer_draw_text_grid(r, rcol, 2, 0xFF8888FF, seg); rcol += (int)strlen(seg);
        snprintf(seg, sizeof(seg), "Ale:%-6lld", (long long)state->resources.ale);
        renderer_draw_text_grid(r, rcol, 2, COL_GOLD, seg);
    }

    /* Status row */
    snprintf(buf, sizeof(buf),
             "  Cap: %d  Hire: %dg/%df  Guards: %s  Scholars: %s  Mana/tick: %d",
             dwarf_cap, hire_gold, hire_food,
             wt_lv >= 1 ? "unlocked" : "locked",
             rh_lv >= 1 ? "unlocked" : "locked",
             mw_lv * 2);
    renderer_draw_text_grid(r, UI_COL_MARGIN, 3, COL_DIM, buf);
    renderer_draw_hline_partial(r, 4, 0, DIVIDER_COL, COL_DIM);

    const int FIRST_ROW = 5;
    const int LAST_ROW  = 40;   /* exclusive-ish bound for content */
    const int ENTRY_H   = 4;    /* rows per upgrade entry incl. spacer */
    const int CAT_H     = 1;    /* rows per category header */

    /* --- Pass 1: compute each entry's row offset (relative, unscrolled) --- */
    int entry_row[UPGR_COUNT];
    int entry_total = 0;
    {
        const char *cat = NULL;
        for (int i = 0; i < UPGR_COUNT; i++) {
            if (cat != upgrades[i].category) {
                cat = upgrades[i].category;
                entry_total += CAT_H;
            }
            entry_row[i] = entry_total;
            entry_total += ENTRY_H;
        }
    }

    /* --- Clamp scroll so selected entry is visible --- */
    int visible_rows = LAST_ROW - FIRST_ROW;
    int sel_row   = entry_row[ui_upgr_cursor];
    int sel_bottom = sel_row + ENTRY_H;
    if (sel_row < ui_upgr_scroll)
        ui_upgr_scroll = sel_row;
    if (sel_bottom > ui_upgr_scroll + visible_rows)
        ui_upgr_scroll = sel_bottom - visible_rows;
    if (ui_upgr_scroll < 0) ui_upgr_scroll = 0;
    int max_scroll = entry_total - visible_rows;
    if (max_scroll < 0) max_scroll = 0;
    if (ui_upgr_scroll > max_scroll) ui_upgr_scroll = max_scroll;

    /* Scroll indicator */
    if (max_scroll > 0) {
        snprintf(buf, sizeof(buf), "  (scroll %d/%d)", ui_upgr_scroll, max_scroll);
        renderer_draw_text_grid(r, DIVIDER_COL - 18, 0, COL_DIM, buf);
    }

    row = FIRST_ROW;
    const char *last_cat = NULL;

    for (int i = 0; i < UPGR_COUNT; i++) {
        const UpgradeInfo *u = &upgrades[i];
        int level  = (int)UPGR_LEVEL(state->upgrades.tier1, i);
        int next   = level + 1;
        int maxed  = (level >= u->max_level);
        int sel    = (i == ui_upgr_cursor);

        /* Category separator */
        int cat_visible_row = -1;
        if (last_cat != u->category) {
            last_cat = u->category;
            cat_visible_row = FIRST_ROW + (entry_row[i] - CAT_H) - ui_upgr_scroll;
            if (cat_visible_row >= FIRST_ROW && cat_visible_row < LAST_ROW) {
                snprintf(buf, sizeof(buf), "--- %s ---", u->category);
                renderer_draw_text_grid(r, UI_COL_MARGIN, cat_visible_row, COL_DIM, buf);
            }
        }

        /* Recompute this entry's screen row from its absolute offset */
        row = FIRST_ROW + entry_row[i] - ui_upgr_scroll;

        /* Skip entirely if off-screen */
        if (row + ENTRY_H <= FIRST_ROW || row >= LAST_ROW) continue;

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
        int cost_wood  = u->cost_wood  * next;
        int cost_food  = u->cost_food  * next;

        int can_afford = !maxed
            && state->resources.gold  >= cost_gold
            && state->resources.stone >= cost_stone
            && state->resources.mana  >= cost_mana
            && (cost_wood == 0 || state->resources.wood >= cost_wood)
            && (cost_food == 0 || state->resources.food >= cost_food);

        /* Check degraded flag for this building */
        int is_degraded = 0;
        if (i == UPGR_WATCH_TOWER) is_degraded = (state->flags & FLAG_WATCH_DEGRADED) != 0;
        if (i == UPGR_RUNE_HALLS)  is_degraded = (state->flags & FLAG_RUNE_DEGRADED)  != 0;
        if (i == UPGR_MANA_WELL)   is_degraded = (state->flags & FLAG_MANA_DEGRADED)  != 0;

        /* Name + level bar */
        if (is_degraded)
            snprintf(buf, sizeof(buf), "%s %-18s %s  Lv %d/%d [DEGRADED]",
                     sel ? ">" : " ", u->name, bar, level, u->max_level);
        else
            snprintf(buf, sizeof(buf), "%s %-18s %s  Lv %d/%d",
                     sel ? ">" : " ", u->name, bar, level, u->max_level);

        uint32_t name_col = is_degraded ? 0xFF4444FF
                          : sel         ? COL_ACCENT
                          :               COL_FG;
        if (row >= FIRST_ROW && row < LAST_ROW)
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, name_col, buf);
        row++;

        /* Next level effect — only show what the NEXT level does */
        if (row >= FIRST_ROW && row < LAST_ROW) {
            if (maxed) {
                renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                        COL_FOOD, "    [MAXED OUT]");
            } else {
                snprintf(buf, sizeof(buf), "    Next: %s",
                         u->level_effects[level]);   /* level = current, so index = next-1 */
                renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, buf);
            }
        }
        row++;

        /* Cost */
        if (!maxed && row >= FIRST_ROW && row < LAST_ROW) {
            char costbuf[64];
            int  off = 0;
            off += snprintf(costbuf+off, sizeof(costbuf)-off, "%d gold  %d stone", cost_gold, cost_stone);
            if (cost_wood > 0)
                off += snprintf(costbuf+off, sizeof(costbuf)-off, "  %d wood", cost_wood);
            if (cost_food > 0)
                off += snprintf(costbuf+off, sizeof(costbuf)-off, "  %d food", cost_food);
            if (cost_mana > 0)
                off += snprintf(costbuf+off, sizeof(costbuf)-off, "  %d mana", cost_mana);
            snprintf(buf, sizeof(buf), "    Cost: %s", costbuf);
            renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                    can_afford ? COL_GOLD : COL_DIM, buf);
        }
    }

    renderer_draw_hline_partial(r, 41, 0, DIVIDER_COL, COL_DIM);
    renderer_draw_text_grid(r, UI_COL_MARGIN, 42, COL_DIM,
        "  [U] Back    [UP/DN] Navigate    [ENTER] Buy");
}
