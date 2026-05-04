#include "ui_research.h"
#include "../render/renderer.h"
#include "../../include/asmoria.h"
#include <stdio.h>
#include <string.h>

/* Layout constants duplicated from ui.h to avoid include conflict */
#define _DIVIDER_COL    90
#define _UI_COL_MARGIN   2

int ui_research_cursor = 0;

typedef struct {
    const char *name;
    const char *effect;
    int         max_stacks;
    int         mana_cost;
} RuneInfo;

static const RuneInfo runes[RUNE_COUNT] = {
    { "Rune of Endurance", "-1 fatigue gain/tick for all dwarves",    RUNE_MAX_SMALL, RUNE_COST_ENDURANCE },
    { "Rune of Plenty",    "+5% resource yield per stack",            RUNE_MAX_SMALL, RUNE_COST_PLENTY    },
    { "Rune of Swiftness", "+1 XP/tick for all working dwarves",      RUNE_MAX_SMALL, RUNE_COST_SWIFTNESS  },
    { "Rune of Warding",   "-1 morale damage from negative events",   RUNE_MAX_LARGE, RUNE_COST_WARDING    },
    { "Rune of Kinship",   "-2 ticks from morale idle recovery rate", RUNE_MAX_LARGE, RUNE_COST_KINSHIP    },
    { "Rune of the Deep",  "Future: unlocks deep tunnels (coming soon)", RUNE_MAX_LARGE, RUNE_COST_DEEP       },
};

void ui_research_move(int delta) {
    ui_research_cursor += delta;
    if (ui_research_cursor < 0)           ui_research_cursor = RUNE_COUNT - 1;
    if (ui_research_cursor >= RUNE_COUNT) ui_research_cursor = 0;
}

int ui_research_selected(void) { return ui_research_cursor; }

void ui_draw_research(Renderer *r, const GameState *state) {
    char buf[128];
    int row = 0;

    int rune_halls_lv = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_RUNE_HALLS);
    int unlocked = (rune_halls_lv >= 5);

    renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_ACCENT,
        "[ RESEARCH : RUNES ]    UP/DN navigate    ENTER inscribe    R close");
    renderer_draw_hline_partial(r, 1, 0, _DIVIDER_COL, COL_DIM);

    /* Resources */
    char seg[32];
    int rcol = _UI_COL_MARGIN;
    snprintf(seg, sizeof(seg), "Gold:%-6lld  ", (long long)state->resources.gold);
    renderer_draw_text_grid(r, rcol, 2, COL_GOLD, seg); rcol += (int)strlen(seg);
    snprintf(seg, sizeof(seg), "Stone:%-6lld  ", (long long)state->resources.stone);
    renderer_draw_text_grid(r, rcol, 2, COL_STONE, seg); rcol += (int)strlen(seg);
    snprintf(seg, sizeof(seg), "Mana:%-6lld", (long long)state->resources.mana);
    renderer_draw_text_grid(r, rcol, 2, COL_MANA, seg);

    if (!unlocked) {
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 4, COL_DIM,
            "  Requires Rune Halls Level 5 to unlock research.");
        snprintf(buf, sizeof(buf), "  Current Rune Halls level: %d / 5", rune_halls_lv);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 5, COL_DIM, buf);
        renderer_draw_hline_partial(r, 6, 0, _DIVIDER_COL, COL_DIM);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 7, COL_DIM, "  [R] Back");
        return;
    }

    renderer_draw_hline_partial(r, 3, 0, _DIVIDER_COL, COL_DIM);
    row = 4;

    for (int i = 0; i < RUNE_COUNT; i++) {
        const RuneInfo *ru = &runes[i];
        int stacks    = (int)RUNE_LEVEL(state->upgrades.tier2, i);
        int maxed     = (stacks >= ru->max_stacks);
        int sel       = (i == ui_research_cursor);
        int can_afford = !maxed && state->resources.mana >= (ru->mana_cost * (stacks + 1));

        char bar[10];
        bar[0] = '[';
        for (int s = 0; s < ru->max_stacks && s < 8; s++)
            bar[s+1] = (s < stacks) ? '#' : '.';
        bar[ru->max_stacks + 1] = ']';
        bar[ru->max_stacks + 2] = '\0';

        snprintf(buf, sizeof(buf), "%s %-24s %s  %d/%d",
                 sel ? ">" : " ", ru->name, bar, stacks, ru->max_stacks);
        uint32_t name_col = maxed ? COL_FOOD : sel ? COL_ACCENT : COL_FG;
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, name_col, buf);
        row++;

        snprintf(buf, sizeof(buf), "    %s", ru->effect);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM, buf);
        row++;

        int scaled_cost = ru->mana_cost * (stacks + 1);
        int can_afford2  = !maxed && state->resources.mana >= scaled_cost;
        if (maxed) {
            renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_FOOD, "    [FULLY INSCRIBED]");
        } else {
            snprintf(buf, sizeof(buf), "    Cost: %d mana  (base %d x stack %d)",
                     scaled_cost, ru->mana_cost, stacks + 1);
            renderer_draw_text_grid(r, _UI_COL_MARGIN, row,
                                    can_afford2 ? COL_MANA : COL_DIM, buf);
        }
        row++;
        row++; /* spacer */

        if (row > 40) break;
    }

    renderer_draw_hline_partial(r, 41, 0, _DIVIDER_COL, COL_DIM);
    renderer_draw_text_grid(r, _UI_COL_MARGIN, 42, COL_DIM,
        "  [R] Back    [UP/DN] Navigate    [ENTER] Inscribe rune");
}
