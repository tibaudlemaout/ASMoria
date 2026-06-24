#include "ui_wonder.h"
#include "ui.h"
#include <stdio.h>

int ui_show_wonder   = 0;
int ui_wonder_cursor = 0;

void ui_wonder_move(int delta) {
    ui_wonder_cursor = (ui_wonder_cursor + delta + WONDER_COUNT) % WONDER_COUNT;
}

int ui_wonder_selected(void) {
    return ui_wonder_cursor;
}

/* ---- cost-check helpers ---- */
static int can_afford_throne(const GameState *s) {
    return s->resources.gold      >= WONDER_THRONE_GOLD  &&
           s->resources.stone     >= WONDER_THRONE_STONE &&
           s->resources.iron_bars >= WONDER_THRONE_BARS  &&
           s->resources.gems      >= WONDER_THRONE_GEMS;
}
static int can_afford_drill(const GameState *s) {
    return s->resources.gold      >= WONDER_DRILL_GOLD     &&
           s->resources.stone     >= WONDER_DRILL_STONE    &&
           s->resources.iron_bars >= WONDER_DRILL_BARS     &&
           s->resources.gems      >= WONDER_DRILL_GEMS     &&
           s->resources.crystals  >= WONDER_DRILL_CRYSTALS;
}
static int can_afford_hammer(const GameState *s) {
    return s->resources.gold      >= WONDER_HAMMER_GOLD     &&
           s->resources.stone     >= WONDER_HAMMER_STONE    &&
           s->resources.iron_bars >= WONDER_HAMMER_BARS     &&
           s->resources.relics    >= WONDER_HAMMER_RELICS   &&
           s->resources.crystals  >= WONDER_HAMMER_CRYSTALS;
}

static int alive_count(const GameState *s) {
    int n = 0;
    for (int i = 0; i < MAX_DWARVES; i++)
        if (s->dwarves[i].alive) n++;
    return n;
}

void ui_draw_wonder(Renderer *r, const GameState *state) {
    char buf[128];
    int  row = UI_ROW_DWARVES;
    int  ac  = alive_count(state);

    /* Extract active wonder state from prestige._pad */
    uint32_t pad      = state->prestige._pad;
    uint8_t  active   = WONDER_ACTIVE(pad);
    uint16_t timer    = WONDER_TIMER(pad);

    int done[3] = {
        (int)WONDER_DONE(state->upgrades.tier2, WONDER_THRONE),
        (int)WONDER_DONE(state->upgrades.tier2, WONDER_DRILL),
        (int)WONDER_DONE(state->upgrades.tier2, WONDER_HAMMER),
    };
    int total_done = done[0] + done[1] + done[2];

    renderer_draw_hline_partial(r, row, 0, DIVIDER_COL, COL_DIM);
    row++;

    snprintf(buf, sizeof(buf), "  [ WORLD WONDERS  %d / 3 ]", total_done);
    renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                            total_done == 3 ? COL_GOLD : COL_ACCENT, buf);
    row += 2;

    /* ---- Wonder rows ---- */
    static const char *wnames[3] = {
        "Throne of the Mountain King",
        "World Drill",
        "Divine Hammer"
    };
    static const int min_dwf[3] = {
        WONDER_THRONE_MIN_DWF,
        WONDER_DRILL_MIN_DWF,
        WONDER_HAMMER_MIN_DWF
    };
    static const int ticks_req[3] = {
        WONDER_THRONE_TICKS,
        WONDER_DRILL_TICKS,
        WONDER_HAMMER_TICKS
    };

    for (int i = 0; i < WONDER_COUNT; i++) {
        /* Status tag */
        char status[40];
        uint32_t scol;
        int has_bp = (int)WONDER_HAS_BP(state->upgrades.tier2, i);
        if (done[i]) {
            snprintf(status, sizeof(status), "[COMPLETE]");
            scol = COL_GOLD;
        } else if (active == (uint8_t)i) {
            int pct = (int)(100 - (timer * 100 / ticks_req[i]));
            snprintf(status, sizeof(status), "[%3d%%  %d ticks left]", pct, timer);
            scol = COL_ACCENT;
        } else if (active != WONDER_NONE_VAL) {
            snprintf(status, sizeof(status), "[other wonder building]");
            scol = COL_DIM;
        } else if (has_bp) {
            snprintf(status, sizeof(status), "[blueprint ready]");
            scol = COL_FOOD;
        } else {
            snprintf(status, sizeof(status), "[no blueprint]");
            scol = COL_DIM;
        }

        /* Cursor */
        const char *prefix = (i == ui_wonder_cursor) ? "> " : "  ";
        uint32_t ncol = done[i] ? COL_GOLD : (i == ui_wonder_cursor ? COL_ACCENT : COL_FG);

        snprintf(buf, sizeof(buf), "%s%s", prefix, wnames[i]);
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, ncol, buf);
        renderer_draw_text_grid(r, UI_COL_MARGIN + 46, row, scol, status);
        row += 2;
    }
    row++;

    /* ---- Detail for selected wonder ---- */
    int sel = ui_wonder_cursor;
    renderer_draw_hline_partial(r, row, 0, DIVIDER_COL, COL_DIM);
    row++;

    if (done[sel]) {
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_GOLD,
            "  This wonder stands complete. Glory to the clan.");
        row++;
    } else {
        /* Cost lines */
        if (sel == WONDER_THRONE) {
            snprintf(buf, sizeof(buf), "  Cost: %d gold  %d stone  %d iron bars  %d gems",
                WONDER_THRONE_GOLD, WONDER_THRONE_STONE, WONDER_THRONE_BARS, WONDER_THRONE_GEMS);
        } else if (sel == WONDER_DRILL) {
            snprintf(buf, sizeof(buf), "  Cost: %d gold  %d stone  %d bars  %d gems  %d crystals",
                WONDER_DRILL_GOLD, WONDER_DRILL_STONE, WONDER_DRILL_BARS,
                WONDER_DRILL_GEMS, WONDER_DRILL_CRYSTALS);
        } else {
            snprintf(buf, sizeof(buf), "  Cost: %d gold  %d stone  %d bars  %d relics  %d crystals",
                WONDER_HAMMER_GOLD, WONDER_HAMMER_STONE, WONDER_HAMMER_BARS,
                WONDER_HAMMER_RELICS, WONDER_HAMMER_CRYSTALS);
        }

        int affordable = (sel == WONDER_THRONE) ? can_afford_throne(state) :
                         (sel == WONDER_DRILL)  ? can_afford_drill(state)  :
                                                   can_afford_hammer(state);
        renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                affordable ? COL_FOOD : 0xFF4444FF, buf);
        row++;

        snprintf(buf, sizeof(buf), "  Needs: %d dwarves (you have %d)  |  %d ticks to build",
                 min_dwf[sel], ac, ticks_req[sel]);
        renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                ac >= min_dwf[sel] ? COL_DIM : 0xFF4444FF, buf);
        row++;

        /* Hint */
        if (active == (uint8_t)sel) {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_ACCENT,
                "  [X] Cancel construction (no refund)");
        } else if (active == WONDER_NONE_VAL && affordable && ac >= min_dwf[sel]) {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_ACCENT,
                "  [ENTER] Begin construction");
        } else if (active != WONDER_NONE_VAL) {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM,
                "  Cancel the active wonder first");
        } else {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM,
                "  Gather more resources or dwarves");
        }
        row++;
    }

    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM,
        "  [↑/↓] Navigate  [J/ESC] Close");
}
