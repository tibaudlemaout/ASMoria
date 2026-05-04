#include "ui_upgrades.h"
#include "ui_research.h"
#include "ui_breach.h"
#include "ui.h"
#include "events_text.h"
#include <stdio.h>
#include <string.h>
 
static const char *job_names[] = {
    "Idle", "Miner", "Lumberer", "Farmer", "Guard", "Scholar"
};
 
/* =========================================================
 * UI state
 * ========================================================= */
int ui_selected_dwarf = -1;
int ui_show_upgrades  = 0;     /* -1 = none selected */
int ui_show_research  = 0;     /* -1 = none selected */
int ui_show_breach    = 0;     /* -1 = none selected */
static int scroll_offset      = 0;
static int dwarf_scroll_offset = 0;
 
void ui_dwarf_scroll(int delta) {
    dwarf_scroll_offset += delta;
    if (dwarf_scroll_offset < 0) dwarf_scroll_offset = 0;
}
 
/* Alive dwarf index cache — rebuilt each draw call */
static int  alive_list[MAX_DWARVES];
static int  alive_list_count = 0;
 
void ui_dwarf_select(int delta) {
    if (alive_list_count == 0) return;
 
    /* Find current position in alive list */
    int pos = -1;
    for (int i = 0; i < alive_list_count; i++) {
        if (alive_list[i] == ui_selected_dwarf) { pos = i; break; }
    }
 
    if (pos < 0) {
        /* Not in list — select first or last */
        pos = (delta > 0) ? 0 : alive_list_count - 1;
    } else {
        pos += delta;
        if (pos < 0)                  pos = alive_list_count - 1;
        if (pos >= alive_list_count)  pos = 0;
    }
 
    ui_selected_dwarf = alive_list[pos];
 
    /* Auto-scroll to keep selection visible */
    int max_visible = UI_ROW_CMDBAR - 2 - (UI_ROW_DWARVES + 2);
    if (pos < dwarf_scroll_offset)
        dwarf_scroll_offset = pos;
    if (pos >= dwarf_scroll_offset + max_visible)
        dwarf_scroll_offset = pos - max_visible + 1;
}
 
void ui_log_scroll(int delta) {
    scroll_offset += delta;
    if (scroll_offset < 0) scroll_offset = 0;
}
 
/* =========================================================
 * Helpers
 * ========================================================= */
static void make_bar(char *out, uint8_t val, uint8_t max) {
    int filled = (int)val * 8 / (int)max;
    out[0] = '[';
    for (int i = 0; i < 8; i++)
        out[i + 1] = (i < filled) ? '#' : '.';
    out[9] = ']'; out[10] = '\0';
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
 
static int wordwrap(const char *text, int line_w,
                    char out[][128], int max_lines) {
    int n = 0, len = (int)strlen(text), pos = 0;
    while (pos < len && n < max_lines) {
        int remain = len - pos;
        if (remain <= line_w) {
            strncpy(out[n], text + pos, remain);
            out[n][remain] = '\0';
            n++; break;
        }
        int cut = line_w;
        while (cut > 0 && text[pos + cut] != ' ') cut--;
        if (cut == 0) cut = line_w;
        strncpy(out[n], text + pos, cut);
        out[n][cut] = '\0';
        n++;
        pos += cut;
        while (pos < len && text[pos] == ' ') pos++;
    }
    return n;
}
 
/* =========================================================
 * Hit-test: which dwarf row did the user click?
 * Returns 0-based dwarf index among alive dwarves that are
 * rendered, or -1 if the click wasn't on a dwarf row.
 * ========================================================= */
int ui_dwarf_at_pixel(Renderer *r, const GameState *state, int px, int py) {
    /* clicks must be in the left panel */
    if (px >= DIVIDER_COL * r->glyph_w) return -1;
 
    int first_row = UI_ROW_DWARVES + 2;   /* first data row */
    int click_row = py / r->glyph_h;
 
    if (click_row < first_row) return -1;
 
    /* walk alive dwarves to find which one occupies that row */
    int row = first_row;
    int skipped = 0;
    for (int i = 0; i < MAX_DWARVES; i++) {
        if (!state->dwarves[i].alive) continue;
        if (skipped < dwarf_scroll_offset) { skipped++; continue; }
        if (row == click_row) return i;
        if (++row > UI_ROW_CMDBAR - 2) break;
    }
    return -1;
}
 
/* =========================================================
 * Master draw
 * ========================================================= */
void ui_draw_all(Renderer *r, const GameState *state) {
    if (ui_show_upgrades) {
        ui_draw_upgrades(r, state);
        ui_draw_divider(r);
        ui_draw_eventlog(r, state);
        return;
    }
    if (ui_show_research) {
        ui_draw_research(r, state);
        ui_draw_divider(r);
        ui_draw_eventlog(r, state);
        return;
    }
    if (ui_show_breach) {
        ui_draw_breach(r, state);
        ui_draw_divider(r);
        ui_draw_eventlog(r, state);
        return;
    }
    ui_draw_titlebar(r, state);
    ui_draw_resources(r, state);
    ui_draw_dwarves(r, state);
    ui_draw_cmdbar(r, state);
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
    int degraded = state->flags & (FLAG_WATCH_DEGRADED|FLAG_RUNE_DEGRADED|FLAG_MANA_DEGRADED);
    snprintf(buf, sizeof(buf), "Depth: %-4u   Tick: %llu%s",
             state->depth, (unsigned long long)state->tick,
             degraded ? "   [!] BUILDING DEGRADED" : "");
    renderer_draw_text_grid(r, 20, UI_ROW_TITLE, degraded ? 0xFF4444FF : COL_DIM, buf);
    renderer_draw_hline_partial(r, UI_ROW_TITLE + 1, 0, DIVIDER_COL, COL_DIM);
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
 
    renderer_draw_hline_partial(r, UI_ROW_RES + 3, 0, DIVIDER_COL, COL_DIM);
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
 
    int bar_lv = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_BARRACKS);
    int dwarf_cap = DWARF_CAP_BASE + bar_lv * DWARF_CAP_PER_LEVEL;
    if (alive == 0) {
        snprintf(buf, sizeof(buf), "[ DWARVES  0 / %d ]", dwarf_cap);
        renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES, COL_FG, buf);
        renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES + 1,
                                COL_DIM, "Your halls are empty. Press H to hire.");
        return;
    }
 
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES + 1,
                            COL_DIM, "Name         Job        Morale      Fatigue     TLv Lv  Progress    XP");
 
    /* Rebuild alive list cache for keyboard navigation */
    alive_list_count = 0;
    for (int i = 0; i < MAX_DWARVES; i++)
        if (state->dwarves[i].alive) alive_list[alive_list_count++] = i;
    int alive_count = alive_list_count;
 
    /* Clamp scroll offset */
    int max_visible = UI_ROW_CMDBAR - 2 - (UI_ROW_DWARVES + 2);
    int max_dscroll = alive_count - max_visible;
    if (max_dscroll < 0) max_dscroll = 0;
    if (dwarf_scroll_offset > max_dscroll) dwarf_scroll_offset = max_dscroll;
 
    /* Scroll indicator in header */
    if (dwarf_scroll_offset > 0 || alive_count > max_visible) {
        snprintf(buf, sizeof(buf), "[ DWARVES  %d / %d ]  PgUp/PgDn to scroll (%d-%d/%d)",
                 alive, dwarf_cap,
                 dwarf_scroll_offset + 1,
                 dwarf_scroll_offset + max_visible < alive_count
                     ? dwarf_scroll_offset + max_visible : alive_count,
                 alive_count);
    } else {
        snprintf(buf, sizeof(buf), "[ DWARVES  %d / %d ]", alive, dwarf_cap);
    }
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES, COL_FG, buf);
 
    int row = UI_ROW_DWARVES + 2;
    int skipped = 0;
    for (int i = 0; i < MAX_DWARVES; i++) {
        const Dwarf *d = &state->dwarves[i];
        if (!d->alive) continue;
        if (skipped < dwarf_scroll_offset) { skipped++; continue; }
 
        int is_selected = (i == ui_selected_dwarf);
 
        const char *job = (d->job <= JOB_SCHOLAR) ? job_names[d->job] : "???";
        char job_label[16];
        if (d->job == JOB_IDLE && d->prev_job != JOB_IDLE)
            snprintf(job_label, sizeof(job_label), "%-10s", "Resting!");
        else
            snprintf(job_label, sizeof(job_label), "%-10s", job);
 
        make_bar(mor_bar, d->morale,  100);
        make_bar(fat_bar, d->fatigue, 100);
 
        /* Total level = sum of all job levels */
        int total_lv = 0;
        for (int j = 0; j < JOB_COUNT; j++)
            total_lv += d->job_level[j];
 
        /* Current job level and XP */
        int cur_lv     = d->job_level[d->job];
        int64_t cur_xp = d->job_xp[d->job];
 
        const char *dname = asm_get_dwarf_name(d->name_idx);
        snprintf(buf, sizeof(buf), "%s%-12s %s",
                 is_selected ? ">" : " ", dname, job_label);
 
        uint32_t row_col = is_selected ? COL_ACCENT
                         : (d->job == JOB_IDLE && d->prev_job != JOB_IDLE)
                           ? 0xFF4444FF : COL_FG;
 
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, row_col, buf);
 
        int bar_col = UI_COL_MARGIN + 24;
        renderer_draw_text_grid(r, bar_col, row, COL_DIM, "Mor:");
        bar_col += 4;
        renderer_draw_text_grid(r, bar_col, row, morale_color(d->morale), mor_bar);
        bar_col += 12;
        renderer_draw_text_grid(r, bar_col, row, COL_DIM, "Fat:");
        bar_col += 4;
        renderer_draw_text_grid(r, bar_col, row, fatigue_color(d->fatigue), fat_bar);
        bar_col += 12;
 
        /* XP progress bar toward next level */
        static const int64_t xp_thresholds[] = {500, 1200, 2500, 4500, 0};
        int64_t xp_next = (cur_lv < MAX_JOB_LEVEL) ? xp_thresholds[cur_lv] : 0;
        int64_t xp_prev = (cur_lv > 0)             ? xp_thresholds[cur_lv-1] : 0;
 
        char xp_bar[12];
        if (cur_lv >= MAX_JOB_LEVEL) {
            /* maxed — solid bar */
            xp_bar[0] = '[';
            for (int x = 0; x < 8; x++) xp_bar[x+1] = '#';
            xp_bar[9] = ']'; xp_bar[10] = ' ';
        } else {
            int64_t span  = xp_next - xp_prev;
            int64_t prog  = cur_xp  - xp_prev;
            int     filled = (span > 0) ? (int)(prog * 8 / span) : 0;
            if (filled < 0) filled = 0;
            if (filled > 8) filled = 8;
            xp_bar[0] = '[';
            for (int x = 0; x < 8; x++) xp_bar[x+1] = (x < filled) ? '#' : '.';
            xp_bar[9] = ']'; xp_bar[10] = ' ';
        }
 
        /* TLv:N  Lv%d bar  xp/next */
        snprintf(buf, sizeof(buf), "T%d ", total_lv);
        renderer_draw_text_grid(r, bar_col, row, COL_DIM, buf);
        bar_col += 3;
 
        uint32_t lv_col = (cur_lv >= MAX_JOB_LEVEL) ? COL_ACCENT : COL_GOLD;
        snprintf(buf, sizeof(buf), "Lv%d", cur_lv);
        renderer_draw_text_grid(r, bar_col, row, lv_col, buf);
        bar_col += 3;
 
        renderer_draw_text_grid(r, bar_col, row, lv_col, xp_bar);
        bar_col += 12;
 
        if (cur_lv < MAX_JOB_LEVEL) {
            snprintf(buf, sizeof(buf), "%lld/%lld", (long long)cur_xp, (long long)xp_next);
        } else {
            snprintf(buf, sizeof(buf), "MAX");
        }
        renderer_draw_text_grid(r, bar_col, row, COL_DIM, buf);
 
        if (++row > UI_ROW_CMDBAR - 2) {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, "...");
            break;
        }
    }
}
 
/* =========================================================
 * Command bar
 * ========================================================= */
void ui_draw_cmdbar(Renderer *r, const GameState *state) {
    renderer_draw_hline_partial(r, UI_ROW_CMDBAR - 1, 0, DIVIDER_COL, COL_DIM);
 
    char line1[256], line2[128];
 
    /* Compute live hire cost */
    int rec_lv    = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_RECRUITERS);
    int alive_cnt = 0;
    for (int i = 0; i < MAX_DWARVES; i++)
        if (state->dwarves[i].alive) alive_cnt++;
    int hire_gold = HIRE_GOLD_BASE - rec_lv * HIRE_GOLD_DISCOUNT + alive_cnt * 5;
    int hire_food = HIRE_FOOD_BASE - rec_lv * HIRE_FOOD_DISCOUNT + alive_cnt * 2;
    if (hire_gold < 10) hire_gold = 10;
    if (hire_food < 5)  hire_food = 5;
 
    /* Line 1: hire info */
    int can_hire = (state->resources.gold >= hire_gold &&
                    state->resources.food >= hire_food);
    /* Breach indicator */
    const char *breach_hint =
        state->raid.active == RAID_COMBAT  ? "  [B] BREACH(!)" :
        state->raid.active == RAID_WARNING ? "  [B] Breach(warn)" :
        "  [B] Breach";
    uint32_t breach_col =
        state->raid.active == RAID_COMBAT  ? 0xFF4444FF :
        state->raid.active == RAID_WARNING ? COL_GOLD   : COL_DIM;
 
    snprintf(line1, sizeof(line1),
             "[H] Hire (%d gold, %d food)%s  [U] Upgrades  [R] Research%s  [F5] Save  [F9] Load",
             hire_gold, hire_food,
             can_hire ? "" : "  [need resources]",
             breach_hint);
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_CMDBAR,
                            can_hire ? COL_FG : COL_DIM, line1);
    (void)breach_col;
 
    /* Line 2: job assignment (context-sensitive) */
    if (ui_selected_dwarf >= 0 && ui_selected_dwarf < MAX_DWARVES
        && state->dwarves[ui_selected_dwarf].alive) {
        const Dwarf *d = &state->dwarves[ui_selected_dwarf];
        if (d->prev_job != JOB_IDLE) {
            const char *sname = asm_get_dwarf_name(d->name_idx);
            snprintf(line2, sizeof(line2),
                     "%s is resting — cannot assign job", sname);
            renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_CMDBAR + 1,
                                    COL_DIM, line2);
        } else {
            const char *sname2 = asm_get_dwarf_name(d->name_idx);
            int can_feed = (state->resources.food >= FEED_FOOD_COST);
            snprintf(line2, sizeof(line2),
                     "%s: [M]iner [L]umberer [F]armer [G]uard [S]cholar [I]dle  %s",
                     sname2,
                     can_feed ? "[E] Feed (10 food)" : "[E] Feed (need food)");
            renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_CMDBAR + 1,
                                    COL_ACCENT, line2);
        }
    } else {
        renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_CMDBAR + 1,
                                COL_DIM, "UP/DN select dwarf  LEFT/RIGHT scroll list  PgUp/PgDn scroll log");
    }
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
typedef struct { char text[128]; uint32_t color; } WrappedLine;
static WrappedLine wlines[LOG_MAX_WRAPPED];
 
void ui_draw_eventlog(Renderer *r, const GameState *state) {
    const EventLog *log = &state->event_log;
    int log_w = (WIN_W / r->glyph_w) - LOG_COL_START - 1;
    if (log_w < 10) log_w = 10;
    if (log_w > 127) log_w = 127;
 
    int total_wrapped = 0;
    int count = log->count;
 
    if (count > 0) {
        int oldest = ((int)log->head - count + EVENT_LOG_SIZE) % EVENT_LOG_SIZE;
        for (int i = 0; i < count && total_wrapped < LOG_MAX_WRAPPED; i++) {
            int slot = (oldest + i) % EVENT_LOG_SIZE;
            const EventRecord *e = &log->entries[slot];
 
            char who[16];
            if (e->dwarf_idx == 0xFF) {
                who[0] = '\0';
            } else {
                uint8_t nidx = state->dwarves[e->dwarf_idx].name_idx;
                const char *wname = asm_get_dwarf_name(nidx);
                snprintf(who, sizeof(who), "%s", wname);
            }
 
            const char *tmpl = evt_get_template(e->code);
            char full[256];
            if (who[0]) snprintf(full, sizeof(full), tmpl, who);
            else        snprintf(full, sizeof(full), "%s", tmpl);
 
            char chunks[8][128];
            int nchunks = wordwrap(full, log_w, chunks, 8);
            uint32_t color = evt_color(e->severity);
 
            for (int c = 0; c < nchunks && total_wrapped < LOG_MAX_WRAPPED; c++) {
                strncpy(wlines[total_wrapped].text, chunks[c], 127);
                wlines[total_wrapped].text[127] = '\0';
                wlines[total_wrapped].color = (c == 0) ? color
                    : (color & 0xFFFFFF00) | (((color & 0xFF) * 2 / 3) & 0xFF);
                total_wrapped++;
            }
        }
    }
 
    int max_scroll = total_wrapped - LOG_VISIBLE_LINES;
    if (max_scroll < 0) max_scroll = 0;
    if (scroll_offset > max_scroll) scroll_offset = max_scroll;
 
    char header[64];
    if (scroll_offset > 0)
        snprintf(header, sizeof(header), "[ EVENT LOG ] ^ %d up (UP/DN)", scroll_offset);
    else
        snprintf(header, sizeof(header), "[ EVENT LOG ]");
    renderer_draw_text_grid(r, LOG_COL_START, LOG_HEADER_ROW, COL_ACCENT, header);
    renderer_draw_hline(r, LOG_HEADER_ROW + 1, COL_DIM);
 
    if (total_wrapped == 0) {
        renderer_draw_text_grid(r, LOG_COL_START, LOG_START_ROW,
                                COL_DIM, "The halls are quiet...");
        return;
    }
 
    int start_line = total_wrapped - LOG_VISIBLE_LINES - scroll_offset;
    if (start_line < 0) start_line = 0;
 
    int screen_row = LOG_START_ROW;
    for (int i = start_line;
         i < total_wrapped && screen_row < LOG_START_ROW + LOG_VISIBLE_LINES;
         i++, screen_row++) {
        renderer_draw_text_grid(r, LOG_COL_START, screen_row,
                                wlines[i].color, wlines[i].text);
    }
 
    if (scroll_offset > 0)
        renderer_draw_text_grid(r, LOG_COL_START,
                                LOG_START_ROW + LOG_VISIBLE_LINES,
                                COL_DIM, "[ UP/DN to scroll ]");
}
