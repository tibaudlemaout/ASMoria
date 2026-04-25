#include "ui.h"
#include "events_text.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static const char *job_names[] = {
    "Idle", "Miner", "Lumberer", "Farmer", "Guard", "Scholar"
};

/* =========================================================
 * Scroll state — pure UI, not in GameState
 * scroll_offset: how many wrapped lines from the bottom we
 * are scrolled up. 0 = newest at bottom (default).
 * ========================================================= */
static int scroll_offset = 0;

void ui_log_scroll(int delta) {
    scroll_offset += delta;
    if (scroll_offset < 0) scroll_offset = 0;
    /* upper bound clamped in ui_draw_eventlog after wrap */
}

/* =========================================================
 * Word-wrap a string into fixed-width lines.
 * out      : array of char[line_w+1] buffers
 * max_lines: size of out array
 * returns  : number of lines written
 * ========================================================= */
static int wordwrap(const char *text, int line_w,
                    char out[][128], int max_lines) {
    int n = 0;
    int len = (int)strlen(text);
    int pos = 0;

    while (pos < len && n < max_lines) {
        /* how many chars fit on this line? */
        int remain = len - pos;
        if (remain <= line_w) {
            /* last chunk — fits entirely */
            strncpy(out[n], text + pos, remain);
            out[n][remain] = '\0';
            n++;
            break;
        }

        /* find last space within line_w chars */
        int cut = line_w;
        while (cut > 0 && text[pos + cut] != ' ') cut--;

        if (cut == 0) {
            /* no space found — hard cut */
            cut = line_w;
        }

        strncpy(out[n], text + pos, cut);
        out[n][cut] = '\0';
        n++;

        /* skip the space */
        pos += cut;
        while (pos < len && text[pos] == ' ') pos++;
    }

    return n;
}

/* =========================================================
 * Colour helpers
 * ========================================================= */
static void make_bar(char *out, uint8_t val, uint8_t max) {
    int filled = (int)val * 8 / (int)max;
    out[0] = '[';
    for (int i = 0; i < 8; i++)
        out[i + 1] = (i < filled) ? '#' : '.';
    out[9]  = ']';
    out[10] = '\0';
}

static uint32_t morale_color(uint8_t m) {
    if (m >= 70) return COL_FOOD;
    if (m >= 40) return COL_GOLD;
    return 0xFF4444FF;
}

static uint32_t fatigue_color(uint8_t f) {
    if (f < 40) return COL_FOOD;
    if (f < 70) return COL_GOLD;
    return 0xFF4444FF;
}

static uint32_t evt_color(uint8_t severity) {
    switch (severity) {
        case EVT_POSITIVE:  return COL_FOOD;
        case EVT_NEGATIVE:  return 0xFF4444FF;
        case EVT_MILESTONE: return COL_ACCENT;
        default:            return COL_FG;
    }
}

/* =========================================================
 * Master draw
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

    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES + 1,
                            COL_DIM, "#   Job        Morale      Fatigue     XP");

    int row = UI_ROW_DWARVES + 2;
    for (int i = 0; i < MAX_DWARVES; i++) {
        const Dwarf *d = &state->dwarves[i];
        if (!d->alive) continue;

        const char *job = (d->job <= JOB_SCHOLAR) ? job_names[d->job] : "???";
        char job_label[16];
        if (d->job == JOB_IDLE && d->prev_job != JOB_IDLE)
            snprintf(job_label, sizeof(job_label), "%-10s", "Resting!");
        else
            snprintf(job_label, sizeof(job_label), "%-10s", job);

        make_bar(mor_bar, d->morale,  100);
        make_bar(fat_bar, d->fatigue, 100);

        snprintf(buf, sizeof(buf), "%-3d %s", i + 1, job_label);
        uint32_t job_col = (d->job == JOB_IDLE && d->prev_job != JOB_IDLE)
                         ? 0xFF4444FF : COL_FG;
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, job_col, buf);

        int bar_col = UI_COL_MARGIN + 15;
        renderer_draw_text_grid(r, bar_col, row, COL_DIM, "Mor:");
        bar_col += 4;
        renderer_draw_text_grid(r, bar_col, row, morale_color(d->morale), mor_bar);
        bar_col += 12;
        renderer_draw_text_grid(r, bar_col, row, COL_DIM, "Fat:");
        bar_col += 4;
        renderer_draw_text_grid(r, bar_col, row, fatigue_color(d->fatigue), fat_bar);
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
 * Event log panel — word-wrapped + scrollable
 *
 * Algorithm:
 *   1. Iterate ring buffer oldest->newest, word-wrap each
 *      event into a flat array of {line, color} pairs.
 *   2. Clamp scroll_offset so we can't scroll past the top.
 *   3. Render the window [total - visible - scroll .. end].
 *   4. If scrolled up, show a scroll indicator in the header.
 * ========================================================= */

typedef struct {
    char     text[128];
    uint32_t color;
} WrappedLine;

static WrappedLine wlines[LOG_MAX_WRAPPED];

void ui_draw_eventlog(Renderer *r, const GameState *state) {
    const EventLog *log = &state->event_log;

    /* Column width available for log text */
    int log_w = (WIN_W / r->glyph_w) - LOG_COL_START - 1;
    if (log_w < 10) log_w = 10;
    if (log_w > 127) log_w = 127;

    /* --- Build wrapped line list from ring buffer --- */
    int total_wrapped = 0;
    int count = log->count;

    if (count > 0) {
        int oldest = ((int)log->head - count + EVENT_LOG_SIZE) % EVENT_LOG_SIZE;

        for (int i = 0; i < count && total_wrapped < LOG_MAX_WRAPPED; i++) {
            int slot = (oldest + i) % EVENT_LOG_SIZE;
            const EventRecord *e = &log->entries[slot];

            char who[16];
            if (e->dwarf_idx == 0xFF)
                who[0] = '\0';
            else
                snprintf(who, sizeof(who), "Dwarf #%d", e->dwarf_idx + 1);

            const char *tmpl = evt_get_template(e->code);
            char full[256];
            if (who[0])
                snprintf(full, sizeof(full), tmpl, who);
            else
                snprintf(full, sizeof(full), "%s", tmpl);

            /* Word-wrap this event into one or more lines */
            char chunks[8][128];
            int nchunks = wordwrap(full, log_w, chunks, 8);
            uint32_t color = evt_color(e->severity);

            for (int c = 0; c < nchunks && total_wrapped < LOG_MAX_WRAPPED; c++) {
                strncpy(wlines[total_wrapped].text, chunks[c], 127);
                wlines[total_wrapped].text[127] = '\0';
                /* Continuation lines slightly dimmed */
                wlines[total_wrapped].color = (c == 0) ? color
                    : (color & 0xFFFFFF00) | (((color & 0xFF) * 2 / 3) & 0xFF);
                total_wrapped++;
            }
        }
    }

    /* Clamp scroll so we can't go past the oldest line */
    int max_scroll = total_wrapped - LOG_VISIBLE_LINES;
    if (max_scroll < 0) max_scroll = 0;
    if (scroll_offset > max_scroll) scroll_offset = max_scroll;

    /* --- Draw header --- */
    char header[64];
    if (scroll_offset > 0)
        snprintf(header, sizeof(header), "[ EVENT LOG ] ^ %d lines up (UP/DN)", scroll_offset);
    else
        snprintf(header, sizeof(header), "[ EVENT LOG ]");
    renderer_draw_text_grid(r, LOG_COL_START, LOG_HEADER_ROW,
                            scroll_offset > 0 ? COL_ACCENT : COL_ACCENT, header);
    renderer_draw_hline(r, LOG_HEADER_ROW + 1, COL_DIM);

    /* --- Draw visible window --- */
    if (total_wrapped == 0) {
        renderer_draw_text_grid(r, LOG_COL_START, LOG_START_ROW,
                                COL_DIM, "The halls are quiet...");
        return;
    }

    /* Window: show lines [start_line .. start_line + LOG_VISIBLE_LINES) */
    int start_line = total_wrapped - LOG_VISIBLE_LINES - scroll_offset;
    if (start_line < 0) start_line = 0;

    int screen_row = LOG_START_ROW;
    for (int i = start_line;
         i < total_wrapped && screen_row < LOG_START_ROW + LOG_VISIBLE_LINES;
         i++, screen_row++) {
        renderer_draw_text_grid(r, LOG_COL_START, screen_row,
                                wlines[i].color, wlines[i].text);
    }

    /* Scroll indicator at bottom if not at newest */
    if (scroll_offset > 0) {
        renderer_draw_text_grid(r, LOG_COL_START,
                                LOG_START_ROW + LOG_VISIBLE_LINES,
                                COL_DIM, "[ UP/DN to scroll - DN for latest ]");
    }
}