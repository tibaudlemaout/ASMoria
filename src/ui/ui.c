#include "ui.h"
#include "events_text.h"
#include <stdio.h>
#include <string.h>

static const char *job_names[] = {
    "Idle", "Miner", "Lumberer", "Farmer", "Guard", "Scholar"
};

/* Build a small ASCII bar like [####....] out of 8 chars */
static void make_bar(char *out, uint8_t val, uint8_t max) {
    int filled = (int)val * 8 / (int)max;
    out[0] = '[';
    for (int i = 0; i < 8; i++)
        out[i + 1] = (i < filled) ? '#' : '.';
    out[9] = ']';
    out[10] = '\0';
}

/* Colour for a morale bar */
static uint32_t morale_color(uint8_t morale) {
    if (morale >= 70) return COL_FOOD;      /* green  */
    if (morale >= 40) return COL_GOLD;      /* yellow */
    return 0xFF4444FF;                      /* red    */
}

/* Colour for a fatigue bar */
static uint32_t fatigue_color(uint8_t fatigue) {
    if (fatigue < 40)  return COL_FOOD;     /* green  */
    if (fatigue < 70)  return COL_GOLD;     /* yellow */
    return 0xFF4444FF;                      /* red    */
}

/* Severity -> colour */
static uint32_t evt_color(uint8_t severity) {
    switch (severity) {
        case EVT_POSITIVE:  return COL_FOOD;
        case EVT_NEGATIVE:  return 0xFF4444FF;
        case EVT_MILESTONE: return COL_ACCENT;
        default:            return COL_FG;
    }
}

/* =========================================================
 * Master draw call
 * ========================================================= */
void ui_draw_all(Renderer *r, const GameState *state) {
    ui_draw_titlebar(r, state);
    ui_draw_resources(r, state);
    ui_draw_dwarves(r, state);
    ui_draw_divider(r);
    ui_draw_eventlog(r, state);
}

/* =========================================================
 * Title bar
 * ========================================================= */
void ui_draw_titlebar(Renderer *r, const GameState *state) {
    char buf[64];
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_TITLE,
                            COL_ACCENT, "ASMoria");
    snprintf(buf, sizeof(buf), "Depth: %-4u   Tick: %llu",
             state->depth, (unsigned long long)state->tick);
    renderer_draw_text_grid(r, 20, UI_ROW_TITLE, COL_DIM, buf);
    renderer_draw_hline(r, UI_ROW_TITLE + 1, COL_DIM);
}

/* =========================================================
 * Resource panel
 * ========================================================= */
void ui_draw_resources(Renderer *r, const GameState *state) {
    char seg[48];
    int col = UI_COL_MARGIN;

    renderer_draw_text_grid(r, col, UI_ROW_RES, COL_FG, "[ RESOURCES ]");

    snprintf(seg, sizeof(seg), "Gold:  %-8lld  ", (long long)state->resources.gold);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_GOLD, seg);
    col += (int)strlen(seg);

    snprintf(seg, sizeof(seg), "Stone: %-8lld  ", (long long)state->resources.stone);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_STONE, seg);
    col += (int)strlen(seg);

    snprintf(seg, sizeof(seg), "Wood:  %-8lld", (long long)state->resources.wood);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_WOOD, seg);

    col = UI_COL_MARGIN;
    snprintf(seg, sizeof(seg), "Food:  %-8lld  ", (long long)state->resources.food);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 2, COL_FOOD, seg);
    col += (int)strlen(seg);

    snprintf(seg, sizeof(seg), "Mana:  %-8lld", (long long)state->resources.mana);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 2, COL_MANA, seg);

    renderer_draw_hline(r, UI_ROW_RES + 3, COL_DIM);
}

/* =========================================================
 * Dwarf panel
 *
 * Layout per dwarf row:
 *   #  Job       Mor:[########]  Fat:[########]  XP
 * ========================================================= */
void ui_draw_dwarves(Renderer *r, const GameState *state) {
    char buf[128];
    char mor_bar[12], fat_bar[12];
    int alive = 0;

    for (int i = 0; i < MAX_DWARVES; i++)
        if (state->dwarves[i].alive) alive++;

    snprintf(buf, sizeof(buf), "[ DWARVES  %d / %d ]", alive, MAX_DWARVES);
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES, COL_FG, buf);

    if (alive == 0) {
        renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES + 1,
                                COL_DIM, "Your halls are empty.");
        return;
    }

    /* Header */
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES + 1, COL_DIM,
                            "#   Job        Morale      Fatigue     XP");

    int row = UI_ROW_DWARVES + 2;
    for (int i = 0; i < MAX_DWARVES; i++) {
        const Dwarf *d = &state->dwarves[i];
        if (!d->alive) continue;

        const char *job = (d->job <= JOB_SCHOLAR) ? job_names[d->job] : "???";

        /* Mark dwarves resting due to exhaustion */
        char job_label[16];
        if (d->job == JOB_IDLE && d->prev_job != JOB_IDLE)
            snprintf(job_label, sizeof(job_label), "%-10s", "Resting!");
        else
            snprintf(job_label, sizeof(job_label), "%-10s", job);

        make_bar(mor_bar, d->morale,  100);
        make_bar(fat_bar, d->fatigue, 100);

        /* Draw label + job */
        snprintf(buf, sizeof(buf), "%-3d %s", i + 1, job_label);
        uint32_t job_col = (d->job == JOB_IDLE && d->prev_job != JOB_IDLE)
                         ? 0xFF4444FF : COL_FG;
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, job_col, buf);

        /* Draw morale bar inline */
        int bar_col = UI_COL_MARGIN + 15;
        renderer_draw_text_grid(r, bar_col, row, COL_DIM, "Mor:");
        bar_col += 4;
        renderer_draw_text_grid(r, bar_col, row, morale_color(d->morale), mor_bar);

        /* Draw fatigue bar inline */
        bar_col += 12;
        renderer_draw_text_grid(r, bar_col, row, COL_DIM, "Fat:");
        bar_col += 4;
        renderer_draw_text_grid(r, bar_col, row, fatigue_color(d->fatigue), fat_bar);

        /* XP */
        bar_col += 12;
        snprintf(buf, sizeof(buf), "XP:%-6lld", (long long)d->xp);
        renderer_draw_text_grid(r, bar_col, row, COL_DIM, buf);

        if (++row > 36) {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, "...");
            break;
        }
    }

    renderer_draw_hline(r, row + 1, COL_DIM);
}

/* =========================================================
 * Vertical divider
 * ========================================================= */
void ui_draw_divider(Renderer *r) {
    int total_rows = WIN_H / r->glyph_h;
    for (int row = 0; row < total_rows; row++)
        renderer_draw_text_grid(r, DIVIDER_COL, row, COL_DIM, "|");
}

/* =========================================================
 * Event log panel
 * ========================================================= */
void ui_draw_eventlog(Renderer *r, const GameState *state) {
    const EventLog *log = &state->event_log;

    renderer_draw_text_grid(r, LOG_COL_START, LOG_HEADER_ROW,
                            COL_ACCENT, "[ EVENT LOG ]");
    renderer_draw_hline(r, LOG_HEADER_ROW + 1, COL_DIM);

    int count = log->count;
    if (count == 0) {
        renderer_draw_text_grid(r, LOG_COL_START, LOG_START_ROW,
                                COL_DIM, "The halls are quiet...");
        return;
    }

    int to_show = count < LOG_VISIBLE_LINES ? count : LOG_VISIBLE_LINES;
    int oldest  = ((int)log->head - to_show + EVENT_LOG_SIZE) % EVENT_LOG_SIZE;
    int max_cols = (WIN_W / r->glyph_w) - LOG_COL_START - 1;
    if (max_cols < 8) max_cols = 8;

    int screen_row = LOG_START_ROW;
    for (int i = 0; i < to_show; i++) {
        int slot = (oldest + i) % EVENT_LOG_SIZE;
        const EventRecord *e = &log->entries[slot];

        char who[16];
        if (e->dwarf_idx == 0xFF)
            who[0] = '\0';
        else
            snprintf(who, sizeof(who), "Dwarf #%d", e->dwarf_idx + 1);

        const char *tmpl = evt_get_template(e->code);
        char line[256];
        if (who[0])
            snprintf(line, sizeof(line), tmpl, who);
        else
            snprintf(line, sizeof(line), "%s", tmpl);

        if ((int)strlen(line) >= max_cols)
            line[max_cols - 1] = '\0';

        renderer_draw_text_grid(r, LOG_COL_START, screen_row,
                                evt_color(e->severity), line);
        screen_row++;
    }
}