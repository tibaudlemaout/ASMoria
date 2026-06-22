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
    int         cost_wood;
    int         cost_food;
    int         depth_req;
    int         workshop_req;
    const char *level_effects[5];
} UpgradeInfo;

static const UpgradeInfo upgrades[UPGR_COUNT] = {
    { "Pick Quality", "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS, 0, UPGR_COST_WOOD_TOOLS, 0, 0, 0,
      { "+1 stone & gold/miner", "+2", "+3",
        "+4 (5 iron bars)", "+5 (15 iron bars)" } },

    { "Saw Quality", "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS, 0, UPGR_COST_WOOD_TOOLS, 0, 0, 0,
      { "+1 wood/lumberer", "+2", "+3",
        "+4 (5 iron bars)", "+5 (15 iron bars)" } },

    { "Irrigation", "TOOLS", UPGR_MAX_TOOLS,
      UPGR_COST_GOLD_TOOLS, UPGR_COST_STONE_TOOLS, 0, UPGR_COST_WOOD_TOOLS, 0, 0, 0,
      { "+1 food/farmer", "+2", "+3",
        "+4 (5 iron bars)", "+5 (15 iron bars)" } },

    { "Barracks", "WORKFORCE", UPGR_MAX_WORKFORCE,
      UPGR_COST_GOLD_WORK, UPGR_COST_STONE_WORK, 0, 0, 0, 0, 0,
      { "Dwarf cap: 24", "Dwarf cap: 32", "Dwarf cap: 40" } },

    { "Recruiters", "WORKFORCE", UPGR_MAX_WORKFORCE,
      UPGR_COST_GOLD_WORK, UPGR_COST_STONE_WORK, 0, 0, 0, 0, 0,
      { "Hire cost -10%", "Hire cost -20%", "Hire cost -30%" } },

    { "Watch Tower", "INFRASTRUCTURE", UPGR_MAX_WATCHTOWER,
      UPGR_COST_GOLD_WATCH, UPGR_COST_STONE_WATCH, 0, 0, 0, 0, 0,
      { "Unlocks Guard job", "Guards -25% neg event dmg",
        "Guards -50% neg event dmg", "Lv4 (3 iron bars) - TBD",
        "Lv5 (8 iron bars) - TBD" } },

    { "Rune Halls", "INFRASTRUCTURE", UPGR_MAX_RUNEHALLS,
      UPGR_COST_GOLD_RUNE, UPGR_COST_STONE_RUNE, UPGR_COST_MANA_RUNE, 0, 0, 0, 0,
      { "Unlocks Scholar", "+1 mana/tick scholars",
        "+1 XP/tick all", "+2 mana/tick scholars", "Unlocks Research" } },

    { "Mana Well", "INFRASTRUCTURE", UPGR_MAX_MANAWELL,
      UPGR_COST_GOLD_MANA, UPGR_COST_STONE_MANA, 0, 0, 0, 0, 0,
      { "+2 mana/tick, cap:600", "+4 mana/tick, cap:1100",
        "+6 mana/tick, cap:1600", "+8 mana/tick (2 gems)",
        "+10 mana/tick (5 gems)" } },

    { "Vault", "STORAGE", UPGR_MAX_STORAGE,
      UPGR_COST_GOLD_VAULT, UPGR_COST_STONE_VAULT, 0, 0, 0, 0, 0,
      { "Gold cap:1000, Relics cap:150, Crystals cap:150",
        "Gold cap:1500, Relics cap:250, Crystals cap:250",
        "Gold cap:2000, Relics cap:350, Crystals cap:350" } },

    { "Warehouse", "STORAGE", UPGR_MAX_STORAGE,
      UPGR_COST_GOLD_WAREHOUSE, UPGR_COST_STONE_WAREHOUSE, 0, UPGR_COST_WOOD_WAREHOUSE, 0, 0, 0,
      { "Stone:1000, Wood:500, Ore:500, Gems:250",
        "Stone:1500, Wood:800, Ore:800, Gems:400",
        "Stone:2000, Wood:1100, Ore:1100, Gems:550" } },

    { "Granary", "STORAGE", UPGR_MAX_STORAGE,
      UPGR_COST_GOLD_GRANARY, UPGR_COST_STONE_GRANARY, 0, 0, UPGR_COST_FOOD_GRANARY, 0, 0,
      { "Food cap: 600", "Food cap: 1000", "Food cap: 1400" } },

    { "Workshop", "CRAFTING", UPGR_MAX_WORKSHOP,
      UPGR_COST_GOLD_WORKSHOP, UPGR_COST_STONE_WORKSHOP, 0, 0, 0, 0, 0,
      { "Unlocks Craftsdwarf. Caps: Bars:100, Wpn:20, Arm:20, Tol:20" } },

    { "Tavern", "CRAFTING", UPGR_MAX_TAVERN,
      500, 400, 0, 200, 0, 0, 0,
      { "1 buff slot, Ale cap:300 (100 iron ore)",
        "2 buff slots, Ale cap:400 (5 iron bars)",
        "3 buff slots, Ale cap:500 (15 iron bars)" } },

    { "Ore Storage", "DEPTH", UPGR_MAX_ORE_STORAGE,
      300, 400, 0, 0, 0, 2, 0,
      { "Ore cap:500, Gems cap:250 (200 iron ore)",
        "Ore cap:800, Gems cap:400 (400 iron ore)",
        "Ore cap:1100, Gems cap:550 (800 iron ore)" } },

    { "Forge", "DEPTH", UPGR_MAX_FORGE,
      500, 300, 0, 0, 0, 2, 1,
      { "Iron bars cap:200 (10 bars)",
        "Iron bars cap:300 (25 bars)",
        "Iron bars cap:400 (50 bars)" } },

    { "Brewery", "CRAFTING", UPGR_MAX_BREWERY,
      400, 0, 0, 300, 0, 0, 1,
      { "Ale cap:400 (100 ale)",
        "Ale cap:600 (250 ale)",
        "Ale cap:800 (500 ale)" } },

    { "Deep Barracks", "DEPTH", UPGR_MAX_DEEP_BARRACKS,
      800, 500, 0, 0, 0, 3, 0,
      { "Dwarf cap +8 (15 bars)",
        "Dwarf cap +16 (35 bars)",
        "Dwarf cap +24 (70 bars)" } },

    { "Relic Vault", "DEPTH", UPGR_MAX_RELIC_VAULT,
      600, 800, 0, 0, 0, 4, 0,
      { "Relics cap:150, Crystals cap:150 (20 relics)",
        "Relics cap:250, Crystals cap:250 (50 relics)",
        "Relics cap:350, Crystals cap:350 (100 relics)" } },

    { "Crystal Conduit", "DEPTH", UPGR_MAX_CRYSTAL_CONDUIT,
      800, 600, 0, 0, 0, 5, 0,
      { "+2 mana/tick from crystals (15 crystals)",
        "+4 mana/tick from crystals (35 crystals)",
        "+6 mana/tick from crystals (70 crystals)" } },

    { "Outposts", "BREACH", UPGR_MAX_OUTPOSTS,
      400, 300, 0, 0, 0, 2, 0,
      { "+50 ticks between raids (5 iron bars)",
        "+100 ticks between raids (10 iron bars)",
        "+150 ticks between raids (20 iron bars)",
        "+200 ticks between raids (40 iron bars)",
        "+250 ticks between raids (75 iron bars)" } },
};

static int last_upgr_delta = 1;

void ui_upgr_move(int delta) {
    last_upgr_delta = delta;
    ui_upgr_cursor += delta;
    if (ui_upgr_cursor < 0)           ui_upgr_cursor = UPGR_COUNT - 1;
    if (ui_upgr_cursor >= UPGR_COUNT) ui_upgr_cursor = 0;
}

int ui_upgr_selected(void) { return ui_upgr_cursor; }

void ui_draw_upgrades(Renderer *r, const GameState *state) {
    char buf[128];
    int row = 0;

    int bar_lv    = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_BARRACKS);
    int db_lv     = (int)UPGR_LEVEL2(state->upgrades.tier2, UPGR_DEEP_BARRACKS);
    int rec_lv    = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_RECRUITERS);
    int wt_lv     = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_WATCH_TOWER);
    int rh_lv     = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_RUNE_HALLS);
    int mw_lv     = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_MANA_WELL);
    int op_lv     = (int)UPGR_LEVEL2(state->upgrades.tier2, UPGR_OUTPOSTS);
    int dwarf_cap = DWARF_CAP_BASE + bar_lv * DWARF_CAP_PER_LEVEL + db_lv * 8;
    int hire_gold = HIRE_GOLD_BASE - rec_lv * HIRE_GOLD_DISCOUNT;
    int hire_food = HIRE_FOOD_BASE - rec_lv * HIRE_FOOD_DISCOUNT;
    if (hire_gold < 10) hire_gold = 10;
    if (hire_food < 5)  hire_food = 5;

    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_ACCENT,
        "[ UPGRADES ]    UP/DN navigate    ENTER buy    U close");
    renderer_draw_hline_partial(r, 1, 0, DIVIDER_COL, COL_DIM);

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

    int ws_lv = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_WORKSHOP);
    if (ws_lv >= 1) {
        snprintf(seg, sizeof(seg), "  Bars:%-5lld  ", (long long)state->resources.iron_bars);
        renderer_draw_text_grid(r, rcol, 2, 0xFF8888FF, seg); rcol += (int)strlen(seg);
        snprintf(seg, sizeof(seg), "Ale:%-5lld", (long long)state->resources.ale);
        renderer_draw_text_grid(r, rcol, 2, COL_GOLD, seg);
    }

    snprintf(buf, sizeof(buf),
             "  Cap: %d  Hire: %dg/%df  Guards: %s  Scholars: %s  Mana/tick: %d  Raid +%dticks",
             dwarf_cap, hire_gold, hire_food,
             wt_lv >= 1 ? "unlocked" : "locked",
             rh_lv >= 1 ? "unlocked" : "locked",
             mw_lv * 2,
             op_lv * 50);
    renderer_draw_text_grid(r, UI_COL_MARGIN, 3, COL_DIM, buf);
    renderer_draw_hline_partial(r, 4, 0, DIVIDER_COL, COL_DIM);

    const int FIRST_ROW = 5;
    const int LAST_ROW  = 40;
    const int ENTRY_H   = 4;
    const int CAT_H     = 1;

    int entry_row[UPGR_COUNT];
    int entry_total = 0;
    {
        const char *cat = NULL;
        for (int i = 0; i < UPGR_COUNT; i++) {
            /* Skip hidden entries in layout calculation too */
            if (upgrades[i].depth_req > 0 && (int)state->depth < upgrades[i].depth_req) {
                entry_row[i] = -1;
                continue;
            }
            if (upgrades[i].workshop_req && !UPGR_LEVEL(state->upgrades.tier1, UPGR_WORKSHOP)) {
                entry_row[i] = -1;
                continue;
            }
            if (cat != upgrades[i].category) {
                cat = upgrades[i].category;
                entry_total += CAT_H;
            }
            entry_row[i] = entry_total;
            entry_total += ENTRY_H;
        }
    }

    /* Clamp cursor to a visible entry, respecting movement direction */
    {
        int safety = UPGR_COUNT;
        while (safety-- > 0 && entry_row[ui_upgr_cursor] < 0) {
            ui_upgr_cursor += (last_upgr_delta >= 0) ? 1 : -1;
            if (ui_upgr_cursor < 0)           ui_upgr_cursor = UPGR_COUNT - 1;
            if (ui_upgr_cursor >= UPGR_COUNT) ui_upgr_cursor = 0;
        }
    }

    int visible_rows = LAST_ROW - FIRST_ROW;
    int sel_row      = (entry_row[ui_upgr_cursor] >= 0) ? entry_row[ui_upgr_cursor] : 0;
    int sel_bottom   = sel_row + ENTRY_H;
    if (sel_row < ui_upgr_scroll)
        ui_upgr_scroll = sel_row;
    if (sel_bottom > ui_upgr_scroll + visible_rows)
        ui_upgr_scroll = sel_bottom - visible_rows;
    if (ui_upgr_scroll < 0) ui_upgr_scroll = 0;
    int max_scroll = entry_total - visible_rows;
    if (max_scroll < 0) max_scroll = 0;
    if (ui_upgr_scroll > max_scroll) ui_upgr_scroll = max_scroll;

    if (max_scroll > 0) {
        snprintf(buf, sizeof(buf), "  (scroll %d/%d)", ui_upgr_scroll, max_scroll);
        renderer_draw_text_grid(r, DIVIDER_COL - 18, 0, COL_DIM, buf);
    }

    const char *last_cat = NULL;

    for (int i = 0; i < UPGR_COUNT; i++) {
        /* Skip hidden upgrades */
        if (entry_row[i] < 0) continue;

        const UpgradeInfo *u = &upgrades[i];
        int level  = (i >= 16)
                   ? (int)UPGR_LEVEL2(state->upgrades.tier2, i)
                   : (int)UPGR_LEVEL(state->upgrades.tier1, i);
        int next   = level + 1;
        int maxed  = (level >= u->max_level);
        int sel    = (i == ui_upgr_cursor);

        /* Category separator */
        if (last_cat != u->category) {
            last_cat = u->category;
            int cat_row = FIRST_ROW + (entry_row[i] - CAT_H) - ui_upgr_scroll;
            if (cat_row >= FIRST_ROW && cat_row < LAST_ROW) {
                snprintf(buf, sizeof(buf), "--- %s ---", u->category);
                renderer_draw_text_grid(r, UI_COL_MARGIN, cat_row, COL_DIM, buf);
            }
        }

        row = FIRST_ROW + entry_row[i] - ui_upgr_scroll;
        if (row + ENTRY_H <= FIRST_ROW || row >= LAST_ROW) continue;

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

        /* Iron bar affordability — checked separately since bars aren't in
           the generic cost fields. Covers Outposts and the ext-cost tiers
           of Watch Tower, Tools, Mana Well, Forge, Deep Barracks, etc. */
        if (can_afford && !maxed) {
            static const int wt_bars[]   = {0,0,0,3,8};
            static const int tool_bars[] = {0,0,0,5,15};
            static const int mw_bars[]   = {0,0,0,2,5};
            static const int op_bars[]   = {5,10,20,40,75};
            static const int forge_bars[]= {10,25,50};
            static const int dbar_bars[] = {15,35,70};
            int bar_cost = 0;
            int lv = level < u->max_level ? level : u->max_level - 1;
            if (i == UPGR_OUTPOSTS      && lv < 5) bar_cost = op_bars[lv];
            else if (i == UPGR_WATCH_TOWER  && lv >= 3) bar_cost = wt_bars[lv];
            else if (i < 3              && lv >= 3) bar_cost = tool_bars[lv];
            else if (i == UPGR_MANA_WELL    && lv >= 3) bar_cost = mw_bars[lv];
            else if (i == UPGR_FORGE        && lv < 3) bar_cost = forge_bars[lv];
            else if (i == UPGR_DEEP_BARRACKS && lv < 3) bar_cost = dbar_bars[lv];
            if (bar_cost > 0 && state->resources.iron_bars < bar_cost)
                can_afford = 0;
        }

        int is_degraded = 0;
        if (i == UPGR_WATCH_TOWER) is_degraded = (state->flags & FLAG_WATCH_DEGRADED) != 0;
        if (i == UPGR_RUNE_HALLS)  is_degraded = (state->flags & FLAG_RUNE_DEGRADED)  != 0;
        if (i == UPGR_MANA_WELL)   is_degraded = (state->flags & FLAG_MANA_DEGRADED)  != 0;

        if (is_degraded)
            snprintf(buf, sizeof(buf), "%s %-18s %s  Lv %d/%d [DEGRADED]",
                     sel ? ">" : " ", u->name, bar, level, u->max_level);
        else
            snprintf(buf, sizeof(buf), "%s %-18s %s  Lv %d/%d",
                     sel ? ">" : " ", u->name, bar, level, u->max_level);

        uint32_t name_col = is_degraded ? 0xFF4444FF : sel ? COL_ACCENT : COL_FG;
        if (row >= FIRST_ROW && row < LAST_ROW)
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, name_col, buf);
        row++;

        if (row >= FIRST_ROW && row < LAST_ROW) {
            if (maxed) {
                renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_FOOD, "    [MAXED OUT]");
            } else {
                const char *eff = (level < u->max_level && u->level_effects[level])
                                  ? u->level_effects[level] : "---";
                snprintf(buf, sizeof(buf), "    Next: %s", eff);
                renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, buf);
            }
        }
        row++;

        if (!maxed && row >= FIRST_ROW && row < LAST_ROW) {
            char costbuf[80];
            int  off = 0;

            if (i < 3 && level >= 3) {
                static const int tg[] = {400,700}, ts[] = {200,350};
                static const int tw[] = {80,150},  tb[] = {5,15};
                int lv = level - 3; if (lv > 1) lv = 1;
                off += snprintf(costbuf+off, sizeof(costbuf)-off,
                    "%dg  %ds  %dw  %d iron bars", tg[lv], ts[lv], tw[lv], tb[lv]);
            } else if (i == UPGR_WATCH_TOWER && level >= 3) {
                static const int wg[] = {400,700}, ws[] = {300,500}, wb[] = {3,8};
                int lv = level - 3; if (lv > 1) lv = 1;
                off += snprintf(costbuf+off, sizeof(costbuf)-off,
                    "%dg  %ds  %d iron bars", wg[lv], ws[lv], wb[lv]);
            } else if (i == UPGR_MANA_WELL && level >= 3) {
                static const int mg[] = {400,700}, ms[] = {200,350}, mgem[] = {2,5};
                int lv = level - 3; if (lv > 1) lv = 1;
                off += snprintf(costbuf+off, sizeof(costbuf)-off,
                    "%dg  %ds  %d gems", mg[lv], ms[lv], mgem[lv]);
            } else if (i == UPGR_TAVERN) {
                static const int og[] = {500,1000,2000}, os[] = {400,800,1500};
                static const int ow[] = {200,400,800};
                int lv = level < 3 ? level : 2;
                off += snprintf(costbuf+off, sizeof(costbuf)-off,
                    "%dg  %ds  %dw", og[lv], os[lv], ow[lv]);
                if (lv == 0)      off += snprintf(costbuf+off, sizeof(costbuf)-off, "  100 iron ore");
                else if (lv == 1) off += snprintf(costbuf+off, sizeof(costbuf)-off, "  5 iron bars");
                else              off += snprintf(costbuf+off, sizeof(costbuf)-off, "  15 iron bars");
            } else if (i == UPGR_ORE_STORAGE) {
                static const int og2[] = {300,600,1000}, os2[] = {400,800,1500};
                static const int oo[]  = {200,400,800};
                int lv = level < 3 ? level : 2;
                off += snprintf(costbuf+off, sizeof(costbuf)-off,
                    "%dg  %ds  %d iron ore", og2[lv], os2[lv], oo[lv]);
            } else if (i == UPGR_FORGE) {
                static const int fg[] = {500,900,1500}, fs[] = {300,600,1000};
                static const int fb[] = {10,25,50};
                int lv = level < 3 ? level : 2;
                off += snprintf(costbuf+off, sizeof(costbuf)-off,
                    "%dg  %ds  %d iron bars", fg[lv], fs[lv], fb[lv]);
            } else if (i == UPGR_BREWERY) {
                static const int bg[] = {400,700,1200}, bw[] = {300,600,1000};
                static const int ba[] = {100,250,500};
                int lv = level < 3 ? level : 2;
                off += snprintf(costbuf+off, sizeof(costbuf)-off,
                    "%dg  %dw  %d ale", bg[lv], bw[lv], ba[lv]);
            } else if (i == UPGR_DEEP_BARRACKS) {
                static const int dg[] = {800,1500,2500}, ds[] = {500,1000,1800};
                static const int db[] = {15,35,70};
                int lv = level < 3 ? level : 2;
                off += snprintf(costbuf+off, sizeof(costbuf)-off,
                    "%dg  %ds  %d iron bars", dg[lv], ds[lv], db[lv]);
            } else if (i == UPGR_RELIC_VAULT) {
                static const int rvg[] = {600,1200,2000}, rvs[] = {800,1600,2800};
                static const int rvr[] = {20,50,100};
                int lv = level < 3 ? level : 2;
                off += snprintf(costbuf+off, sizeof(costbuf)-off,
                    "%dg  %ds  %d relics", rvg[lv], rvs[lv], rvr[lv]);
            } else if (i == UPGR_CRYSTAL_CONDUIT) {
                static const int cg[] = {800,1500,2500}, cs2[] = {600,1200,2000};
                static const int cc[] = {15,35,70};
                int lv = level < 3 ? level : 2;
                off += snprintf(costbuf+off, sizeof(costbuf)-off,
                    "%dg  %ds  %d crystals", cg[lv], cs2[lv], cc[lv]);
            } else if (i == UPGR_OUTPOSTS) {
                static const int og[] = {400,700,1100,1600,2500};
                static const int os[] = {300,500, 800,1200,1800};
                static const int ob[] = {  5, 10,  20,  40,  75};
                int lv = level < 5 ? level : 4;
                off += snprintf(costbuf+off, sizeof(costbuf)-off,
                    "%dg  %ds  %d iron bars", og[lv], os[lv], ob[lv]);
            } else {
                off += snprintf(costbuf+off, sizeof(costbuf)-off,
                    "%dg  %ds", cost_gold, cost_stone);
                if (cost_wood > 0)
                    off += snprintf(costbuf+off, sizeof(costbuf)-off, "  %dw", cost_wood);
                if (cost_food > 0)
                    off += snprintf(costbuf+off, sizeof(costbuf)-off, "  %d food", cost_food);
                if (cost_mana > 0)
                    off += snprintf(costbuf+off, sizeof(costbuf)-off, "  %d mana", cost_mana);
            }

            snprintf(buf, sizeof(buf), "    Cost: %s", costbuf);
            renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                    can_afford ? COL_GOLD : COL_DIM, buf);
        }
    }

    renderer_draw_hline_partial(r, 41, 0, DIVIDER_COL, COL_DIM);
    renderer_draw_text_grid(r, UI_COL_MARGIN, 42, COL_DIM,
        "  [U] Back    [UP/DN] Navigate    [ENTER] Buy");
}
