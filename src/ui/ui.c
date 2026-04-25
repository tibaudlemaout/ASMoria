#include "ui.h"
#include <stdio.h>
#include <string.h>

static const char *job_names[] = {
    "Idle", "Miner", "Lumberer", "Farmer", "Guard", "Scholar"
};

/* =========================================================
 * Master draw call — order matters for layering
 * ========================================================= */

void ui_draw_all(Renderer *r, const GameState *state) {
    ui_draw_titlebar(r, state);
    ui_draw_resources(r, state);
    ui_draw_dwarves(r, state);
}

/* =========================================================
 * Title bar
 * ========================================================= */

void ui_draw_titlebar(Renderer *r, const GameState *state) {
    char buf[128];

    /* Game name left, tick counter right */
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_TITLE,
                            COL_ACCENT, "ASMoria");

    snprintf(buf, sizeof(buf), "Depth: %-4u   Tick: %llu",
             state->depth,
             (unsigned long long)state->tick);
    renderer_draw_text_grid(r, 30, UI_ROW_TITLE, COL_DIM, buf);

    renderer_draw_hline(r, UI_ROW_TITLE + 1, COL_DIM);
}

/* =========================================================
 * Resource panel
 * ========================================================= */

void ui_draw_resources(Renderer *r, const GameState *state) {
    char buf[256];

    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_RES,
                            COL_FG, "[ RESOURCES ]");

    snprintf(buf, sizeof(buf),
             "Gold: %-8lld  Stone: %-8lld  Wood: %-8lld  Food: %-8lld  Mana: %-8lld",
             (long long)state->resources.gold,
             (long long)state->resources.stone,
             (long long)state->resources.wood,
             (long long)state->resources.food,
             (long long)state->resources.mana);

    /* Draw each resource segment in its own colour */
    int col = UI_COL_MARGIN;
    char seg[64];

    snprintf(seg, sizeof(seg), "Gold: %-8lld  ", (long long)state->resources.gold);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_GOLD, seg);
    col += (int)strlen(seg);

    snprintf(seg, sizeof(seg), "Stone: %-8lld  ", (long long)state->resources.stone);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_STONE, seg);
    col += (int)strlen(seg);

    snprintf(seg, sizeof(seg), "Wood: %-8lld  ", (long long)state->resources.wood);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_WOOD, seg);
    col += (int)strlen(seg);

    snprintf(seg, sizeof(seg), "Food: %-8lld  ", (long long)state->resources.food);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_FOOD, seg);
    col += (int)strlen(seg);

    snprintf(seg, sizeof(seg), "Mana: %-8lld", (long long)state->resources.mana);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_MANA, seg);

    renderer_draw_hline(r, UI_ROW_RES + 2, COL_DIM);
}

/* =========================================================
 * Dwarf panel
 * ========================================================= */

void ui_draw_dwarves(Renderer *r, const GameState *state) {
    char buf[128];
    int alive = 0;

    for (int i = 0; i < MAX_DWARVES; i++)
        if (state->dwarves[i].alive) alive++;

    snprintf(buf, sizeof(buf), "[ DWARVES  %d / %d ]", alive, MAX_DWARVES);
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES, COL_FG, buf);

    if (alive == 0) {
        renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES + 1,
                                COL_DIM, "No dwarves yet. Your halls are empty.");
        return;
    }

    /* Header row */
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES + 1, COL_DIM,
        "#   Job        Morale  Fatigue  XP");

    int row = UI_ROW_DWARVES + 2;
    for (int i = 0; i < MAX_DWARVES; i++) {
        const Dwarf *d = &state->dwarves[i];
        if (!d->alive) continue;

        const char *job = (d->job <= JOB_SCHOLAR) ? job_names[d->job] : "???";
        snprintf(buf, sizeof(buf),
                 "%-3d %-10s %-7u %-8u %lld",
                 i + 1, job, d->morale, d->fatigue, (long long)d->xp);
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_FG, buf);
        row++;

        /* Clamp to window height */
        if (row > 36) {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, "...");
            break;
        }
    }
}