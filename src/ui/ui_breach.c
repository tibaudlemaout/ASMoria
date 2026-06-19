#include "ui_breach.h"
#include "../render/renderer.h"
#include "../../include/asmoria.h"
#include <stdio.h>
#include <string.h>

#define _DIVIDER_COL    90
#define _UI_COL_MARGIN   2
#define GRID_CHAR_W      4   /* chars per cell */
#define GRID_START_ROW   7
#define GRID_START_COL   2

/* ---- cursor state for placement ---- */
int ui_breach_cursor_col = 1;
int ui_breach_cursor_row = 0;

void ui_breach_move(int dc, int dr) {
    ui_breach_cursor_col += dc;
    ui_breach_cursor_row += dr;
    if (ui_breach_cursor_col < 1)                   ui_breach_cursor_col = 1;
    if (ui_breach_cursor_col >= RAID_COL_SETTLE) ui_breach_cursor_col = RAID_COL_SETTLE - 1;
    if (ui_breach_cursor_row < 0)                    ui_breach_cursor_row = 0;
    if (ui_breach_cursor_row >= RAID_ROWS)           ui_breach_cursor_row = RAID_ROWS - 1;
}

static const char *place_mode_names[] = {
    "Guard", "Wall (5s+2bar)", "Spike Trap (3bar+1tol)", "Slow Trap (2bar+1gem)"
};

/* ---- HP bar ---- */
static void make_hp_bar(char *out, int hp, int max, int width) {
    int filled = (max > 0 && hp > 0) ? hp * width / max : 0;
    if (filled > width) filled = width;
    out[0] = '[';
    for (int i = 0; i < width; i++)
        out[i+1] = (i < filled) ? '#' : '.';
    out[width+1] = ']';
    out[width+2] = '\0';
}

static uint32_t hp_col(int hp, int max) {
    if (max <= 0) return COL_DIM;
    int pct = hp * 100 / max;
    if (pct > 60) return COL_FOOD;
    if (pct > 30) return COL_GOLD;
    return 0xFF4444FF;
}

/* ---- draw the grid ---- */
static void draw_grid(Renderer *r, const GameState *state, int phase) {
    const Raid *raid = &state->raid;
    char buf[32];

    /* Column header */
    renderer_draw_text_grid(r, GRID_START_COL, GRID_START_ROW - 1,
                            COL_DIM, "     0   1   2   3   4   5   6   7   8   9  10  |");

    for (int row = 0; row < RAID_ROWS; row++) {
        int screen_row = GRID_START_ROW + row;

        /* Row label */
        snprintf(buf, sizeof(buf), "R%d ", row);
        renderer_draw_text_grid(r, GRID_START_COL, screen_row, COL_DIM, buf);

        for (int col = 0; col < RAID_COLS; col++) {
            int sx = GRID_START_COL + 3 + col * GRID_CHAR_W;
            uint8_t cell = raid->grid[row][col];
            int is_cursor = ((phase == RAID_NONE || phase == RAID_WARNING || phase == RAID_COMBAT) &&
                             col == ui_breach_cursor_col &&
                             row == ui_breach_cursor_row);

            /* Check for enemy at this cell */
            int enemy_here = -1;
            for (int e = 0; e < RAID_MAX_ENEMIES; e++) {
                const Enemy *en = &raid->enemies[e];
                if (en->type != ENEMY_NONE &&
                    en->col == col && en->row == row &&
                    en->spawn_delay == 0) {
                    enemy_here = e;
                    break;
                }
            }

            const char *glyph = "  . ";
            uint32_t gcol = COL_DIM;

            if (col == RAID_COL_SETTLE) {
                glyph = "  | ";
                gcol = COL_ACCENT;
            } else if (enemy_here >= 0) {
                const Enemy *en = &raid->enemies[enemy_here];
                switch (en->type) {
                    case ENEMY_GOBLIN_SCOUT:
                    case ENEMY_GOBLIN_RAIDER: glyph = " [g]"; break;
                    case ENEMY_STONE_TROLL:
                    case ENEMY_WAR_TROLL:     glyph = " [T]"; break;
                    case ENEMY_DEMON:          glyph = " [D]"; break;
                    default:                   glyph = " [?]"; break;
                }
                gcol = 0xFF4444FF;
            } else {
                switch (cell) {
                    case CELL_GUARD:      glyph = " [G]"; gcol = COL_FOOD;    break;
                    case CELL_WALL:       glyph = " [W]"; gcol = COL_STONE;   break;
                    case CELL_SPIKE_TRAP: glyph = " [^]"; gcol = COL_GOLD;    break;
                    case CELL_SLOW_TRAP:  glyph = " [~]"; gcol = COL_MANA;    break;
                    case CELL_SETTLEMENT: glyph = "  |" ; gcol = COL_ACCENT;  break;
                    default:              glyph = "  . "; gcol = COL_DIM;     break;
                }
            }

            if (is_cursor) gcol = COL_ACCENT;
            renderer_draw_text_grid(r, sx, screen_row, gcol, glyph);
        }
    }
}

/* ---- status panel beside grid ---- */
static void draw_status(Renderer *r, const GameState *state) {
    const Raid *raid = &state->raid;
    char buf[80];
    char hpbar[16];
    int row = GRID_START_ROW;
    int sx = GRID_START_COL + 3 + RAID_COLS * GRID_CHAR_W + 2;

    /* Guards */
    renderer_draw_text_grid(r, sx, row, COL_FG, "GUARDS:");
    row++;
    for (int i = 0; i < raid->guard_count; i++) {
        const RaidGuard *g = &raid->guards[i];
        if (!g->active) {
            snprintf(buf, sizeof(buf), "  G%d [KO]", i+1);
            renderer_draw_text_grid(r, sx, row, COL_DIM, buf);
        } else {
            make_hp_bar(hpbar, g->hp, g->hp_max, 6);
            snprintf(buf, sizeof(buf), " G%d %s %d/%d",
                     i+1, hpbar, g->hp, g->hp_max);
            renderer_draw_text_grid(r, sx, row,
                                    hp_col(g->hp, g->hp_max), buf);
        }
        row++;
    }

    /* Enemies */
    row++;
    renderer_draw_text_grid(r, sx, row, 0xFF4444FF, "ENEMIES:");
    row++;
    for (int i = 0; i < RAID_MAX_ENEMIES; i++) {
        const Enemy *e = &raid->enemies[i];
        if (e->type == ENEMY_NONE) continue;
        if (e->spawn_delay > 0) {
            snprintf(buf, sizeof(buf), "  [pending %d]", e->spawn_delay);
            renderer_draw_text_grid(r, sx, row, COL_DIM, buf);
        } else {
            make_hp_bar(hpbar, e->hp, e->hp_max, 8);
            snprintf(buf, sizeof(buf), "  %s %d/%d",
                     hpbar, e->hp, e->hp_max);
            renderer_draw_text_grid(r, sx, row,
                                    hp_col(e->hp, e->hp_max), buf);
        }
        row++;
        if (row > GRID_START_ROW + RAID_ROWS + 4) break;
    }
}

/* ---- main draw function ---- */
void ui_draw_breach(Renderer *r, const GameState *state) {
    const Raid *raid = &state->raid;
    char buf[128];
    int row = 0;

    uint32_t hcol = (raid->active == RAID_COMBAT)  ? 0xFF4444FF :
                    (raid->active == RAID_WARNING)  ? COL_GOLD   : COL_DIM;

    renderer_draw_text_grid(r, _UI_COL_MARGIN, row, hcol, "[ THE BREACH ]");
    renderer_draw_hline_partial(r, 1, 0, _DIVIDER_COL, COL_DIM);

    /* ---- NONE ---- */
    if (raid->active == RAID_NONE) {
        uint64_t tl = 0;
        if (raid->next_raid_tick > state->tick &&
            raid->next_raid_tick - state->tick < 10000ULL)
            tl = raid->next_raid_tick - state->tick;
        if (tl > 0)
            snprintf(buf, sizeof(buf),
                "  Raid warning in ~%llu ticks -- prepare your defences now!",
                (unsigned long long)tl);
        else
            snprintf(buf, sizeof(buf), "  Raid warning imminent...");
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 2, COL_DIM, buf);

        int pm = raid->place_mode < 4 ? raid->place_mode : 0;
        snprintf(buf, sizeof(buf), "  Mode: [%s]   [TAB] cycle   [ENTER] place   [X] remove",
                 place_mode_names[pm]);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 3, COL_DIM, buf);
        snprintf(buf, sizeof(buf),
                 "  Bars:%-4lld  Tools:%-4lld  Gems:%-4lld  Stone:%-4lld"
                 "  Walls:%-3lld  Spikes:%-3lld  Slows:%-3lld",
                 (long long)state->resources.iron_bars,
                 (long long)state->resources.tools,
                 (long long)state->resources.gems,
                 (long long)state->resources.stone,
                 (long long)state->resources.walls,
                 (long long)state->resources.spike_traps,
                 (long long)state->resources.slow_traps);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 4, COL_DIM, buf);
        renderer_draw_hline_partial(r, 5, 0, _DIVIDER_COL, COL_DIM);

        draw_grid(r, state, RAID_NONE);

        int lr = GRID_START_ROW + RAID_ROWS + 1;
        renderer_draw_text_grid(r, _UI_COL_MARGIN, lr, COL_DIM,
            "  [G]=Guard  [W]=Wall  [^]=Spike  [~]=Slow  [|]=Settlement");
        renderer_draw_hline_partial(r, 41, 0, _DIVIDER_COL, COL_DIM);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 42, COL_DIM,
            "  Arrows: move   TAB: cycle mode   ENTER: place   X: remove   B: close");
        return;
    }

    /* ---- Threat info ---- */
    int threat = raid->threat;
    if (threat < 1) threat = 1;
    if (threat > 5) threat = 5;
    if (raid->active == RAID_COMBAT)
        snprintf(buf, sizeof(buf), "  Threat: %d/5 - %s    Enemies remaining: %d",
                 threat,
                 (threat <= 2) ? "Goblins" :
                 (threat <= 4) ? "Trolls"  : "Demon",
                 raid->enemies_remaining);
    else
        snprintf(buf, sizeof(buf), "  Threat: %d/5 - %s    Place your defences!",
                 threat,
                 (threat <= 2) ? "Goblins" :
                 (threat <= 4) ? "Trolls"  : "Demon");
    renderer_draw_text_grid(r, _UI_COL_MARGIN, 2,
                            raid->active == RAID_COMBAT ? 0xFF4444FF : COL_GOLD,
                            buf);

    /* ---- WARNING ---- */
    if (raid->active == RAID_WARNING) {
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 3, COL_GOLD,
            "  Place your defences before the raid arrives!");

        int pm = raid->place_mode < 4 ? raid->place_mode : 0;
        snprintf(buf, sizeof(buf), "  Mode: [%s]   [TAB] cycle mode   [ENTER] place   [X] remove",
                 place_mode_names[pm]);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 4, COL_DIM, buf);
        snprintf(buf, sizeof(buf), "  Bars:%-4lld  Tools:%-4lld  Gems:%-4lld  Stone:%-4lld  "
                 "Walls:%-3lld  Spikes:%-3lld  Slows:%-3lld",
                 (long long)state->resources.iron_bars,
                 (long long)state->resources.tools,
                 (long long)state->resources.gems,
                 (long long)state->resources.stone,
                 (long long)state->resources.walls,
                 (long long)state->resources.spike_traps,
                 (long long)state->resources.slow_traps);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 5, COL_DIM, buf);
        renderer_draw_hline_partial(r, 5, 0, _DIVIDER_COL, COL_DIM);

        draw_grid(r, state, RAID_WARNING);

        /* Legend */
        int lr = GRID_START_ROW + RAID_ROWS + 1;
        renderer_draw_text_grid(r, _UI_COL_MARGIN, lr, COL_DIM,
            "  [G]=Guard  [W]=Wall  [^]=Spike Trap  [~]=Slow Trap  [|]=Settlement  [g/T/D]=Enemy");
        renderer_draw_hline_partial(r, 41, 0, _DIVIDER_COL, COL_DIM);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 42, COL_DIM,
            "  Arrows: move cursor   TAB: cycle mode   ENTER: place   X: remove   B: close");
        return;
    }

    /* ---- COMBAT ---- */
    if (raid->active == RAID_COMBAT) {
        if (raid->settlement_breached) {
            renderer_draw_text_grid(r, _UI_COL_MARGIN, 3, 0xFF4444FF,
                "  !! SETTLEMENT BREACHED - resolving...");
        } else {
            renderer_draw_text_grid(r, _UI_COL_MARGIN, 3, 0xFF4444FF,
                "  !! COMBAT IN PROGRESS !!");
        }
        renderer_draw_hline_partial(r, 4, 0, _DIVIDER_COL, COL_DIM);
        snprintf(buf, sizeof(buf), "  Walls: Iron Bars:%-4lld  Gems:%-4lld  Tools:%-4lld",
                 (long long)state->resources.iron_bars,
                 (long long)state->resources.gems,
                 (long long)state->resources.tools);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 4, COL_DIM, buf);
        renderer_draw_hline_partial(r, 5, 0, _DIVIDER_COL, COL_DIM);

        draw_grid(r, state, RAID_COMBAT);
        draw_status(r, state);

        renderer_draw_hline_partial(r, 41, 0, _DIVIDER_COL, COL_DIM);
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 42, COL_DIM,
            "  [R] Retreat   [B] Close panel");
        return;
    }

    /* ---- RESULT ---- */
    if (raid->settlement_breached) {
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 3, 0xFF4444FF,
            "  Raid lost - the settlement was breached. Morale -20 to all dwarves.");
    } else {
        snprintf(buf, sizeof(buf),
                 "  Raid won! Rewards: +%d gold%s",
                 raid->reward_gold,
                 raid->reward_relics > 0 ? "  +relics" : "");
        renderer_draw_text_grid(r, _UI_COL_MARGIN, 3, COL_FOOD, buf);
    }

    renderer_draw_hline_partial(r, 41, 0, _DIVIDER_COL, COL_DIM);
    renderer_draw_text_grid(r, _UI_COL_MARGIN, 42, COL_DIM, "  [B] Close");
}
