#include "ui_prestige.h"
#include "../render/renderer.h"
#include "../../include/asmoria.h"
#include <stdio.h>
#include <string.h>

#define _DIVIDER_COL    90
#define _UI_COL_MARGIN   2

int ui_prestige_cursor = 0;

extern int64_t asm_can_prestige(GameState *state);

typedef struct {
    const char *name;
    const char *desc;
    int         branch;     /* 0=Blessing 1=Beards 2=Reputation */
    int         cost;
    int         prereq;     /* PNODE_* or -1 */
} PNodeInfo;

static const PNodeInfo nodes[PNODE_COUNT] = {
    /* Ancestor's Blessing */
    { "+2% yield",          "All resource yield +2%",             0, 1, -1           },
    { "+4% yield",          "All resource yield +4% (total +6%)", 0, 2, PNODE_YIELD_1 },
    { "+6% yield",          "All resource yield +6% (total +12%)",0, 3, PNODE_YIELD_2 },
    { "Start: +50 gold",    "Begin each run with 50 extra gold",  0, 1, PNODE_YIELD_1 },
    { "Start: +100 gold",   "Begin each run with 100 extra gold", 0, 2, PNODE_START_GOLD_1 },
    { "Start: +50 stone",   "Begin each run with 50 extra stone", 0, 1, PNODE_START_GOLD_1 },
    { "+1 food/tick",       "Passive +1 food per tick",           0, 2, PNODE_START_GOLD_1 },
    { "+1 wood/tick",       "Passive +1 wood per tick",           0, 2, PNODE_FOOD_TICK },
    /* Legendary Beards */
    { "Start: 2 dwarves",   "Begin with 2 dwarves instead of 1", 1, 1, -1             },
    { "Start: 3 dwarves",   "Begin with 3 dwarves",              1, 2, PNODE_START_DWF_2 },
    { "Start: 4 dwarves",   "Begin with 4 dwarves",              1, 3, PNODE_START_DWF_3 },
    { "Hire cost -10%",     "Hiring costs 10% less",             1, 1, PNODE_START_DWF_2 },
    { "Hire cost -25%",     "Hiring costs 25% less total",       1, 2, PNODE_HIRE_10 },
    { "Start: Lv1 Pick",    "Pick Quality starts at level 1",    1, 1, PNODE_HIRE_10 },
    { "Start: Lv1 Saw",     "Saw Quality starts at level 1",     1, 1, PNODE_START_PICK },
    { "Start: Lv1 Irrig.",  "Irrigation starts at level 1",      1, 1, PNODE_START_SAW },
    { "Start: Barracks 1",  "Begin with Barracks level 1",       1, 2, PNODE_START_DWF_3 },
    /* Clan Reputation */
    { "Rune cost -10%",     "All rune research costs 10% less",  2, 1, -1              },
    { "Rune cost -25%",     "All rune research costs 25% less",  2, 2, PNODE_RUNE_10  },
    { "Start: +100 mana",   "Begin each run with 100 mana",      2, 1, PNODE_RUNE_10  },
    { "Start: Rune Halls 1","Begin with Rune Halls level 1",     2, 2, PNODE_START_MANA },
    { "+1 mana/tick",       "Passive +1 mana per tick",          2, 2, PNODE_START_MANA },
    { "+2 mana/tick",       "Passive +2 mana per tick total",    2, 3, PNODE_MANA_TICK_1 },
    { "Start: Scholar",     "Scholar job unlocked from start",   2, 2, PNODE_START_RUNE_H },
};

static const char *branch_names[] = {
    "ANCESTOR'S BLESSING", "LEGENDARY BEARDS", "CLAN REPUTATION"
};

void ui_prestige_move(int delta) {
    ui_prestige_cursor += delta;
    if (ui_prestige_cursor < 0)             ui_prestige_cursor = PNODE_COUNT; /* wrap to Seal */
    if (ui_prestige_cursor > PNODE_COUNT)   ui_prestige_cursor = 0;
}

int ui_prestige_selected(void) { return ui_prestige_cursor; }

void ui_draw_prestige(Renderer *r, const GameState *state) {
    char buf[128];
    int row = 0;
    const PrestigeState *p = &state->prestige;

    renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_ACCENT,
        "[ CLAN HONOR ]    UP/DN navigate    ENTER buy node    P close");
    renderer_draw_hline_partial(r, 1, 0, _DIVIDER_COL, COL_DIM);

    /* Honor and stats */
    snprintf(buf, sizeof(buf),
             "  Honor: %u    Total earned: %u    Prestiges: %u",
             p->honor, p->total_honor, p->total_prestiges);
    renderer_draw_text_grid(r, _UI_COL_MARGIN, 2, COL_GOLD, buf);

    /* Prestige eligibility */
    int can = (int)asm_can_prestige((GameState *)state);
    uint64_t res_k = p->total_resources / 1000;
    snprintf(buf, sizeof(buf),
             "  Resources: %lluK / 10K  |  Raids: %d / 1  |  Ticks: %llu / 1000%s",
             (unsigned long long)res_k,
             state->raid.raids_completed,
             (unsigned long long)state->tick,
             can ? "  [PRESTIGE AVAILABLE — press ENTER on Seal]" : "");
    renderer_draw_text_grid(r, _UI_COL_MARGIN, 3, can ? COL_FOOD : COL_DIM, buf);
    renderer_draw_hline_partial(r, 4, 0, _DIVIDER_COL, COL_DIM);
    row = 5;

    int cur_branch = -1;
    for (int i = 0; i < PNODE_COUNT; i++) {
        const PNodeInfo *n  = &nodes[i];
        int unlocked = PNODE_UNLOCKED(p->nodes, i);
        int prereq_met = (n->prereq < 0) || PNODE_UNLOCKED(p->nodes, n->prereq);
        int can_buy  = !unlocked && prereq_met && (int)p->honor >= n->cost;
        int sel      = (i == ui_prestige_cursor);

        /* Branch header */
        if (n->branch != cur_branch) {
            cur_branch = n->branch;
            snprintf(buf, sizeof(buf), "--- %s ---", branch_names[cur_branch]);
            renderer_draw_text_grid(r, _UI_COL_MARGIN, row, COL_DIM, buf);
            row++;
        }

        /* Node line */
        char status = unlocked ? '#' : (prereq_met ? '.' : 'x');
        snprintf(buf, sizeof(buf), "%s [%c] %-22s  %d honor  %s",
                 sel ? ">" : " ",
                 status,
                 n->name,
                 n->cost,
                 n->desc);

        uint32_t col = unlocked    ? COL_FOOD
                     : !prereq_met ? COL_DIM
                     : can_buy     ? COL_GOLD
                     : sel         ? COL_ACCENT
                     :               COL_FG;
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row, col, buf);
        row++;

        if (row > 41) break;
    }

    /* Seal Chronicles button */
    if (row <= 41) {
        row++;
        renderer_draw_hline_partial(r, row, 0, _DIVIDER_COL, COL_DIM);
        row++;
        int seal_sel = (ui_prestige_cursor == PNODE_COUNT);
        snprintf(buf, sizeof(buf), "%s [SEAL THE CHRONICLES]  Prestige now — earn honor, reset run",
                 seal_sel ? ">" : " ");
        renderer_draw_text_grid(r, _UI_COL_MARGIN, row,
                                can ? 0xFF4444FF : COL_DIM, buf);
    }

    renderer_draw_hline_partial(r, 42, 0, _DIVIDER_COL, COL_DIM);
    renderer_draw_text_grid(r, _UI_COL_MARGIN, 43, COL_DIM,
        "  [#]=owned  [.]=available  [x]=locked  |  [P] Back  [ENTER] Buy/Prestige");
}
