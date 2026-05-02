#include "ui_upgrades.h"
#include "ui.h"
#include <stdio.h>

int ui_upgr_cursor = 0;

typedef struct {
    const char *name;
    const char *desc;
    const char *effect;
} UpgradeInfo;

static const UpgradeInfo upgrades[UPGR_COUNT] = {
    { "Pick Quality",  "Sharper picks for your miners.",
      "+1 stone & +0.5 gold per miner per level" },
    { "Saw Quality",   "Better saws for your lumberers.",
      "+1 wood per lumberer per level"            },
    { "Irrigation",    "Channels and ditches for the farms.",
      "+1 food per farmer per level"              },
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

    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_ACCENT,
        "[ UPGRADES : TOOLS ]    UP/DN navigate    ENTER buy    U close");
    renderer_draw_hline(r, 1, COL_DIM);
    row = 3;

    for (int i = 0; i < UPGR_COUNT; i++) {
        const UpgradeInfo *u = &upgrades[i];
        int level  = (int)UPGR_LEVEL(state->upgrades.tier1, i);
        int next   = level + 1;
        int maxed  = (level >= UPGR_MAX_LEVEL);
        int sel    = (i == ui_upgr_cursor);

        /* Level bar e.g. [###] */
        char bar[8];
        bar[0] = '[';
        for (int l = 0; l < UPGR_MAX_LEVEL; l++)
            bar[l + 1] = (l < level) ? '#' : '.';
        bar[UPGR_MAX_LEVEL + 1] = ']';
        bar[UPGR_MAX_LEVEL + 2] = '\0';

        int cost_gold  = UPGR_COST_GOLD_BASE  * next;
        int cost_stone = UPGR_COST_STONE_BASE * next;
        int can_afford = !maxed
            && state->resources.gold  >= cost_gold
            && state->resources.stone >= cost_stone;

        /* Name row */
        snprintf(buf, sizeof(buf), "%s %-18s %s  Lv %d / %d",
                 sel ? ">" : " ", u->name, bar, level, UPGR_MAX_LEVEL);
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
                     "    Next level costs: %d gold   %d stone",
                     cost_gold, cost_stone);
            renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                    can_afford ? COL_GOLD : COL_DIM, buf);
        }
        row++;

        /* Spacer */
        row++;
    }

    renderer_draw_hline(r, 39, COL_DIM);
    renderer_draw_text_grid(r, UI_COL_MARGIN, 40, COL_DIM,
                            "  [U] Back    [UP/DN] Navigate    [ENTER] Buy upgrade");
}