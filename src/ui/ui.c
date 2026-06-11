#include "ui_upgrades.h"
#include "ui_research.h"
#include "ui_breach.h"
#include "ui_prestige.h"
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
int ui_selected_dwarf    = -1;
int ui_show_upgrades     = 0;
int ui_show_research     = 0;
int ui_show_breach       = 0;
int ui_show_prestige     = 0;
static int scroll_offset      = 0;
static int dwarf_scroll_offset = 0;

static int  alive_list[MAX_DWARVES];
static int  alive_list_count = 0;

void ui_dwarf_scroll(int delta) {
    dwarf_scroll_offset += delta;
    if (dwarf_scroll_offset < 0) dwarf_scroll_offset = 0;
}

void ui_dwarf_select(int delta) {
    if (alive_list_count == 0) return;
    int pos = -1;
    for (int i = 0; i < alive_list_count; i++)
        if (alive_list[i] == ui_selected_dwarf) { pos = i; break; }
    if (pos < 0)
        pos = (delta > 0) ? 0 : alive_list_count - 1;
    else {
        pos += delta;
        if (pos < 0)                 pos = alive_list_count - 1;
        if (pos >= alive_list_count) pos = 0;
    }
    ui_selected_dwarf = alive_list[pos];
    if (pos < dwarf_scroll_offset)
        dwarf_scroll_offset = pos;
    if (pos >= dwarf_scroll_offset + DWARF_LIST_ROWS)
        dwarf_scroll_offset = pos - DWARF_LIST_ROWS + 1;
}

void ui_log_scroll(int delta) {
    scroll_offset += delta;
    if (scroll_offset < 0) scroll_offset = 0;
}

/* =========================================================
 * Helpers
 * ========================================================= */
static void make_bar(char *out, uint8_t val, uint8_t max) {
    int filled = (int)val * 10 / (int)max;
    if (filled < 0) filled = 0;
    if (filled > 10) filled = 10;
    out[0] = '[';
    for (int i = 0; i < 10; i++)
        out[i+1] = (i < filled) ? '#' : '.';
    out[11] = ']';
    out[12] = '\0';
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
            strncpy(out[n], text + pos, 127);
            out[n][127] = '\0';
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
 * Hit-test
 * ========================================================= */
int ui_dwarf_at_pixel(Renderer *r, const GameState *state, int px, int py) {
    if (px >= DIVIDER_COL * r->glyph_w) return -1;
    int first_row = UI_ROW_DWARVES + 2;
    int click_row = py / r->glyph_h;
    if (click_row < first_row || click_row >= UI_ROW_SPLIT) return -1;
    int row = first_row;
    int skipped = 0;
    for (int i = 0; i < MAX_DWARVES; i++) {
        if (!state->dwarves[i].alive) continue;
        if (skipped < dwarf_scroll_offset) { skipped++; continue; }
        if (row == click_row) return i;
        if (++row >= UI_ROW_SPLIT) break;
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
    if (ui_show_prestige) {
        ui_draw_prestige(r, state);
        ui_draw_divider(r);
        ui_draw_eventlog(r, state);
        return;
    }
    ui_draw_titlebar(r, state);
    ui_draw_resources(r, state);
    ui_draw_dwarves(r, state);
    ui_draw_dwarf_detail(r, state);
    ui_draw_cmdbar(r, state);
    ui_draw_divider(r);
    ui_draw_eventlog(r, state);
}

/* =========================================================
 * Title bar
 * ========================================================= */
void ui_draw_titlebar(Renderer *r, const GameState *state) {
    char buf[256];
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_TITLE, COL_ACCENT, "ASMoria");

    int degraded = state->flags & (FLAG_WATCH_DEGRADED|FLAG_RUNE_DEGRADED|FLAG_MANA_DEGRADED);
    uint32_t deep_stacks = (uint32_t)((state->upgrades.tier2 >> (RUNE_DEEP * 4)) & 0xF);
    int can_dig = (state->depth < DEPTH_MAX) && (state->depth <= deep_stacks);

    static const int dig_cost_stone[] = {0, 500, 1500, 3000, 6000};
    static const int dig_cost_gold[]  = {0, 300, 1000, 2500, 5000};
    char dig_hint[64] = "";
    if (can_dig) {
        int next = (int)state->depth;
        int can_afford_dig = (state->resources.stone >= dig_cost_stone[next] &&
                              state->resources.gold  >= dig_cost_gold[next]);
        snprintf(dig_hint, sizeof(dig_hint), "  [D] Dig (%dg/%ds)%s",
                 dig_cost_gold[next], dig_cost_stone[next],
                 can_afford_dig ? "" : " [need resources]");
    } else if (state->depth < DEPTH_MAX) {
        snprintf(dig_hint, sizeof(dig_hint), "  [D] locked");
    }

    snprintf(buf, sizeof(buf), "Depth: %u/%u   Tick: %llu%s%s",
             state->depth,
             (uint32_t)(deep_stacks + 1 <= DEPTH_MAX ? deep_stacks + 1 : DEPTH_MAX),
             (unsigned long long)state->tick,
             dig_hint,
             degraded ? "  [!] DEGRADED" : "");
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

    if (state->depth >= 2) {
        col = UI_COL_MARGIN;
        snprintf(seg, sizeof(seg), "Iron:  %-8lld  ", (long long)state->resources.iron_ore);
        renderer_draw_text_grid(r, col, UI_ROW_RES + 3, 0xFF8888FF, seg);
        col += (int)strlen(seg);
        if (state->depth >= 3) {
            snprintf(seg, sizeof(seg), "Gems:  %-8lld  ", (long long)state->resources.gems);
            renderer_draw_text_grid(r, col, UI_ROW_RES + 3, 0xAAFFAAFF, seg);
            col += (int)strlen(seg);
        }
        if (state->depth >= 4) {
            snprintf(seg, sizeof(seg), "Relics:%-8lld  ", (long long)state->resources.relics);
            renderer_draw_text_grid(r, col, UI_ROW_RES + 3, COL_ACCENT, seg);
            col += (int)strlen(seg);
        }
        if (state->depth >= 5) {
            snprintf(seg, sizeof(seg), "Crystals:%-6lld", (long long)state->resources.crystals);
            renderer_draw_text_grid(r, col, UI_ROW_RES + 3, COL_MANA, seg);
        }
    }

    int sep_row = (state->depth >= 2) ? UI_ROW_RES + 4 : UI_ROW_RES + 3;
    renderer_draw_hline_partial(r, sep_row, 0, DIVIDER_COL, COL_DIM);
}

/* =========================================================
 * Dwarf list (compact)
 * ========================================================= */
void ui_draw_dwarves(Renderer *r, const GameState *state) {
    char buf[128];
    int alive = 0;

    for (int i = 0; i < MAX_DWARVES; i++)
        if (state->dwarves[i].alive) alive++;

    /* Rebuild alive list */
    alive_list_count = 0;
    for (int i = 0; i < MAX_DWARVES; i++)
        if (state->dwarves[i].alive) alive_list[alive_list_count++] = i;

    int bar_lv    = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_BARRACKS);
    int dwarf_cap = DWARF_CAP_BASE + bar_lv * DWARF_CAP_PER_LEVEL;

    /* Clamp scroll */
    int max_dscroll = alive_list_count - DWARF_LIST_ROWS;
    if (max_dscroll < 0) max_dscroll = 0;
    if (dwarf_scroll_offset > max_dscroll) dwarf_scroll_offset = max_dscroll;

    /* Header */
    if (dwarf_scroll_offset > 0 || alive > DWARF_LIST_ROWS)
        snprintf(buf, sizeof(buf), "[ DWARVES  %d / %d ]  PgUp/PgDn (%d-%d/%d)",
                 alive, dwarf_cap, dwarf_scroll_offset + 1,
                 dwarf_scroll_offset + DWARF_LIST_ROWS < alive
                     ? dwarf_scroll_offset + DWARF_LIST_ROWS : alive,
                 alive);
    else
        snprintf(buf, sizeof(buf), "[ DWARVES  %d / %d ]", alive, dwarf_cap);
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES, COL_FG, buf);

    if (alive == 0) {
        renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES + 1,
                                COL_DIM, "Your halls are empty. Press H to hire.");
        return;
    }

    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DWARVES + 1, COL_DIM,
        " Name         Job        Mor   Fat   TLv  Lv");

    int row = UI_ROW_DWARVES + 2;
    int skipped = 0;
    for (int i = 0; i < MAX_DWARVES; i++) {
        const Dwarf *d = &state->dwarves[i];
        if (!d->alive) continue;
        if (skipped < dwarf_scroll_offset) { skipped++; continue; }
        if (row >= UI_ROW_SPLIT - 1) {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, " ...");
            break;
        }

        int is_sel = (i == ui_selected_dwarf);
        const char *job = (d->job <= JOB_SCHOLAR) ? job_names[d->job] : "???";
        char job_label[12];
        if (d->job == JOB_IDLE && d->prev_job != JOB_IDLE)
            snprintf(job_label, sizeof(job_label), "%-10s", "Resting!");
        else
            snprintf(job_label, sizeof(job_label), "%-10s", job);

        int total_lv = 0;
        for (int j = 0; j < JOB_COUNT; j++) total_lv += d->job_level[j];
        int cur_lv = d->job_level[d->job];
        const char *dname = asm_get_dwarf_name(d->name_idx);

        snprintf(buf, sizeof(buf), "%s %-12s %-10s %3d%%  %3d%%  T%-2d  L%d",
                 is_sel ? ">" : " ",
                 dname, job_label,
                 d->morale, d->fatigue,
                 total_lv, cur_lv);

        uint32_t row_col = is_sel ? COL_ACCENT
                         : (d->job == JOB_IDLE && d->prev_job != JOB_IDLE)
                           ? 0xFF4444FF
                           : morale_color(d->morale);

        renderer_draw_text_grid(r, UI_COL_MARGIN, row, row_col, buf);
        row++;
    }
}

/* =========================================================
 * Full-width horizontal split line
 * ========================================================= */
void ui_draw_hline_full(Renderer *r, int row) {
    int total_cols = WIN_W / r->glyph_w;
    for (int c = 0; c < total_cols; c++)
        renderer_draw_text_grid(r, c, row, COL_DIM, "-");
}

/* =========================================================
 * Dwarf detail panel (bottom half, full width)
 * ========================================================= */
void ui_draw_dwarf_detail(Renderer *r, const GameState *state) {
    char buf[128];
    char bar[14];

    ui_draw_hline_full(r, UI_ROW_SPLIT);

    if (ui_selected_dwarf < 0 || ui_selected_dwarf >= MAX_DWARVES
        || !state->dwarves[ui_selected_dwarf].alive) {
        renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DETAIL, COL_DIM,
            " Select a dwarf with UP/DN to view details.");
        return;
    }

    const Dwarf *d = &state->dwarves[ui_selected_dwarf];
    const char *dname = asm_get_dwarf_name(d->name_idx);
    const char *job   = (d->job <= JOB_SCHOLAR) ? job_names[d->job] : "???";

    int total_lv = 0;
    for (int j = 0; j < JOB_COUNT; j++) total_lv += d->job_level[j];
    int cur_lv     = d->job_level[d->job];
    int64_t cur_xp = d->job_xp[d->job];

    /* Row 1: name, job, total level */
    snprintf(buf, sizeof(buf), " %s  —  %s%s                              TLv: %d",
             dname, job,
             (d->job == JOB_IDLE && d->prev_job != JOB_IDLE) ? " (Resting)" : "",
             total_lv);
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DETAIL, COL_ACCENT, buf);

    /* Row 2: morale bar + fatigue bar side by side */
    make_bar(bar, d->morale, 100);
    snprintf(buf, sizeof(buf), " Morale:  %s  %3d%%", bar, d->morale);
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DETAIL + 1,
                            morale_color(d->morale), buf);
    make_bar(bar, d->fatigue, 100);
    snprintf(buf, sizeof(buf), " Fatigue: %s  %3d%%", bar, d->fatigue);
    renderer_draw_text_grid(r, UI_COL_MARGIN + 30, UI_ROW_DETAIL + 1,
                            fatigue_color(d->fatigue), buf);

    /* Row 3: XP progress bar */
    static const int64_t xp_thresh[] = {500, 1200, 2500, 4500, 0};
    int64_t xp_next = (cur_lv < MAX_JOB_LEVEL) ? xp_thresh[cur_lv] : 0;
    int64_t xp_prev = (cur_lv > 0) ? xp_thresh[cur_lv - 1] : 0;
    if (cur_lv >= MAX_JOB_LEVEL) {
        bar[0] = '[';
        for (int x = 0; x < 12; x++) bar[x+1] = '#';
        bar[13] = ']'; bar[13] = '\0';
    } else {
        int64_t span = xp_next - xp_prev;
        int filled = (span > 0) ? (int)((cur_xp - xp_prev) * 12 / span) : 0;
        if (filled < 0) filled = 0;
        if (filled > 12) filled = 12;
        bar[0] = '[';
        for (int x = 0; x < 12; x++) bar[x+1] = (x < filled) ? '#' : '.';
        bar[13] = ']'; bar[13] = '\0';
    }
    if (cur_lv < MAX_JOB_LEVEL)
        snprintf(buf, sizeof(buf), " XP: %s  %lld / %lld   Lv%d  %s",
                 bar, (long long)cur_xp, (long long)xp_next, cur_lv, job);
    else
        snprintf(buf, sizeof(buf), " XP: %s  MAX   Lv%d  %s",
                 bar, cur_lv, job);
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DETAIL + 2, COL_GOLD, buf);

    /* Row 4: per-job levels */
    static const char *jnames[] = {"Idle","Mine","Lumb","Farm","Guard","Scholar"};
    int col = UI_COL_MARGIN;
    renderer_draw_text_grid(r, col, UI_ROW_DETAIL + 3, COL_DIM, " Jobs: ");
    col += 7;
    for (int j = 1; j < JOB_COUNT; j++) {
        snprintf(buf, sizeof(buf), "%s Lv%d  ", jnames[j], d->job_level[j]);
        uint32_t jcol = (j == d->job) ? COL_ACCENT : COL_DIM;
        renderer_draw_text_grid(r, col, UI_ROW_DETAIL + 3, jcol, buf);
        col += 12;
    }

    /* Row 5: job assignment keys */
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DETAIL + 4, COL_DIM,
        " [M]iner  [L]umberer  [F]armer  [G]uard  [S]cholar  [I]dle      [E] Feed (10 food)");
}

/* =========================================================
 * Command bar
 * ========================================================= */
void ui_draw_cmdbar(Renderer *r, const GameState *state) {
    renderer_draw_hline_partial(r, UI_ROW_CMDBAR - 1, 0, DIVIDER_COL, COL_DIM);

    char line1[256];
    int rec_lv    = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_RECRUITERS);
    int alive_cnt = 0;
    for (int i = 0; i < MAX_DWARVES; i++)
        if (state->dwarves[i].alive) alive_cnt++;
    int hire_gold = HIRE_GOLD_BASE - rec_lv * HIRE_GOLD_DISCOUNT + alive_cnt * 5;
    int hire_food = HIRE_FOOD_BASE - rec_lv * HIRE_FOOD_DISCOUNT + alive_cnt * 2;
    if (hire_gold < 10) hire_gold = 10;
    if (hire_food < 5)  hire_food = 5;

    int can_hire = (state->resources.gold >= hire_gold &&
                    state->resources.food >= hire_food);

    const char *breach_hint =
        state->raid.active == RAID_COMBAT  ? "  [B] BREACH(!)" :
        state->raid.active == RAID_WARNING ? "  [B] Breach(warn)" : "  [B] Breach";

    snprintf(line1, sizeof(line1),
             "[H] Hire (%dg/%df)%s  [U] Upgrades  [R] Research  [P] Prestige%s  [F5] Save  [F9] Load",
             hire_gold, hire_food,
             can_hire ? "" : " [need resources]",
             breach_hint);
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_CMDBAR,
                            can_hire ? COL_FG : COL_DIM, line1);
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
            if (e->dwarf_idx == 0xFF) who[0] = '\0';
            else {
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
        snprintf(header, sizeof(header), "[ EVENT LOG ] ^ %d up", scroll_offset);
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
}