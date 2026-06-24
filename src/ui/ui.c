#include "ui_upgrades.h"
#include "ui_research.h"
#include "ui_breach.h"
#include "ui_prestige.h"
#include "ui_craft.h"
#include "ui_tavern.h"
#include "ui_wonder.h"
#include "ui.h"
#include "events_text.h"
#include <stdio.h>
#include <string.h>

static const char *job_names[] = {
    "Idle", "Miner", "Lumberer", "Farmer", "Guard", "Scholar", "Craftsdwarf"
};

/* =========================================================
 * UI state
 * ========================================================= */
int ui_selected_dwarf    = -1;
int ui_show_upgrades     = 0;
int ui_show_research     = 0;
int ui_show_breach       = 0;
int ui_show_prestige     = 0;
int ui_show_craft        = 0;
int ui_show_tavern       = 0;
int ui_show_depth        = 0;
int ui_show_help         = 0;
int ui_show_achievements = 0;
/* ui_show_wonder is defined in ui_wonder.c — do NOT add it here */
int ui_game_won          = 0;
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
    if (ui_show_craft) {
        ui_draw_craft(r, state);
        ui_draw_divider(r);
        ui_draw_eventlog(r, state);
        return;
    }
    if (ui_show_tavern) {
        ui_draw_tavern(r, state);
        ui_draw_divider(r);
        ui_draw_eventlog(r, state);
        return;
    }
    ui_draw_titlebar(r, state);
    ui_draw_resources(r, state);
    if (ui_game_won) {
        ui_draw_win_screen(r, state);
    } else if (ui_show_help) {
        ui_draw_help(r);
    } else if (ui_show_achievements) {
        ui_draw_achievements(r, state);
    } else if (ui_show_wonder) {
        ui_draw_wonder(r, state);
    } else if (ui_show_depth) {
        ui_draw_depth_confirm(r, state);
    } else {
        ui_draw_dwarves(r, state);
        ui_draw_dwarf_detail(r, state);
    }
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

    /* Row 1: the 5 base resources, horizontal */
    col = UI_COL_MARGIN;
    snprintf(seg, sizeof(seg), "Gold: %-7lld ", (long long)state->resources.gold);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_GOLD, seg);
    col += (int)strlen(seg);
    snprintf(seg, sizeof(seg), "Stone: %-7lld ", (long long)state->resources.stone);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_STONE, seg);
    col += (int)strlen(seg);
    snprintf(seg, sizeof(seg), "Wood: %-7lld ", (long long)state->resources.wood);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_WOOD, seg);
    col += (int)strlen(seg);
    snprintf(seg, sizeof(seg), "Food: %-7lld ", (long long)state->resources.food);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_FOOD, seg);
    col += (int)strlen(seg);
    snprintf(seg, sizeof(seg), "Mana: %-7lld", (long long)state->resources.mana);
    renderer_draw_text_grid(r, col, UI_ROW_RES + 1, COL_MANA, seg);

    /* Row 2: depth resources, horizontal, appear as unlocked */
    if (state->depth >= 2) {
        col = UI_COL_MARGIN;
        snprintf(seg, sizeof(seg), "Iron: %-7lld ", (long long)state->resources.iron_ore);
        renderer_draw_text_grid(r, col, UI_ROW_RES + 2, 0xFF8888FF, seg);
        col += (int)strlen(seg);
        if (state->depth >= 3) {
            snprintf(seg, sizeof(seg), "Gems: %-7lld ", (long long)state->resources.gems);
            renderer_draw_text_grid(r, col, UI_ROW_RES + 2, 0xAAFFAAFF, seg);
            col += (int)strlen(seg);
        }
        if (state->depth >= 4) {
            snprintf(seg, sizeof(seg), "Relics: %-7lld ", (long long)state->resources.relics);
            renderer_draw_text_grid(r, col, UI_ROW_RES + 2, COL_ACCENT, seg);
            col += (int)strlen(seg);
        }
        if (state->depth >= 5) {
            snprintf(seg, sizeof(seg), "Crystals: %-6lld  ", (long long)state->resources.crystals);
            renderer_draw_text_grid(r, col, UI_ROW_RES + 2, COL_MANA, seg);
            col += (int)strlen(seg);
        }
    }

    /* Crafted resources — append to row 2 once Workshop is built */
    {
        int ws_lv = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_WORKSHOP);
        if (ws_lv >= 1) {
            if (state->depth < 2) col = UI_COL_MARGIN;
            snprintf(seg, sizeof(seg), "Bars:%-5lld  ", (long long)state->resources.iron_bars);
            renderer_draw_text_grid(r, col, UI_ROW_RES + 2, 0xFF8888FF, seg);
            col += (int)strlen(seg);
            snprintf(seg, sizeof(seg), "Ale:%-5lld  ", (long long)state->resources.ale);
            renderer_draw_text_grid(r, col, UI_ROW_RES + 2, COL_GOLD, seg);
            col += (int)strlen(seg);
            snprintf(seg, sizeof(seg), "Wpn:%-4lld  ", (long long)state->resources.weapons);
            renderer_draw_text_grid(r, col, UI_ROW_RES + 2, 0xFF8888FF, seg);
            col += (int)strlen(seg);
            snprintf(seg, sizeof(seg), "Arm:%-4lld  ", (long long)state->resources.armour);
            renderer_draw_text_grid(r, col, UI_ROW_RES + 2, 0xAAAAFFFF, seg);
            col += (int)strlen(seg);
            snprintf(seg, sizeof(seg), "Tol:%-4lld", (long long)state->resources.tools);
            renderer_draw_text_grid(r, col, UI_ROW_RES + 2, COL_GOLD, seg);
        }
    }

    renderer_draw_hline_partial(r, UI_ROW_RES + 3, 0, DIVIDER_COL, COL_DIM);
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
        const char *job = (d->job < JOB_COUNT) ? job_names[d->job] : "???";
        char job_label[12];
        if (d->job == JOB_IDLE && d->prev_job != JOB_IDLE)
            snprintf(job_label, sizeof(job_label), "%-10s", "Resting!");
        else
            snprintf(job_label, sizeof(job_label), "%-10s", job);

        int total_lv = 0;
        for (int j = 0; j < JOB_COUNT; j++) total_lv += d->job_level[j];
        int cur_lv = d->job_level[d->job];
        const char *dname = asm_get_dwarf_name(d->name_idx);

        int is_resting = (d->job == JOB_IDLE && d->prev_job != JOB_IDLE);

        /* Base line: selector + name + job (white or accent if selected) */
        snprintf(buf, sizeof(buf), "%s %-12s %-10s ",
                 is_sel ? ">" : " ", dname, job_label);
        uint32_t base_col = is_resting ? 0xFF4444FF
                          : is_sel     ? COL_ACCENT
                          :              COL_FG;
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, base_col, buf);

        /* Morale % — coloured */
        int col_off = 1 + 13 + 11; /* selector + name + job */
        char mor_buf[8], fat_buf[8];
        snprintf(mor_buf, sizeof(mor_buf), "%3d%%  ", d->morale);
        snprintf(fat_buf, sizeof(fat_buf), "%3d%%  ", d->fatigue);
        renderer_draw_text_grid(r, UI_COL_MARGIN + col_off, row,
                                morale_color(d->morale), mor_buf);
        renderer_draw_text_grid(r, UI_COL_MARGIN + col_off + 6, row,
                                fatigue_color(d->fatigue), fat_buf);

        /* TLv + Lv — gold and trait tag for heroes */
        if (d->is_hero && d->hero_trait > 0 && d->hero_trait <= TRAIT_COUNT) {
            static const char *trait_names[] = {
                "", "Ironhide", "Berserker", "Scholar-King",
                "Foreman", "Deepborn", "Blessed"
            };
            snprintf(buf, sizeof(buf), "T%-2d  L%d [%s]",
                     total_lv, cur_lv, trait_names[d->hero_trait]);
            renderer_draw_text_grid(r, UI_COL_MARGIN + col_off + 12, row,
                                    COL_GOLD, buf);
        } else {
            snprintf(buf, sizeof(buf), "T%-2d  L%d", total_lv, cur_lv);
            renderer_draw_text_grid(r, UI_COL_MARGIN + col_off + 12, row,
                                    COL_DIM, buf);
        }
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
    const char *job   = (d->job < JOB_COUNT) ? job_names[d->job] : "???";

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

    /* Hero trait banner — shown on the same row, offset right */
    if (d->is_hero && d->hero_trait > 0 && d->hero_trait <= TRAIT_COUNT) {
        static const char *trait_names[] = {
            "", "Ironhide", "Berserker", "Scholar-King",
            "Foreman", "Deepborn", "Blessed"
        };
        static const char *trait_desc[] = {
            "",
            "+20 max HP as guard",
            "Double ATK below 30% HP",
            "Counts as 2 scholars",
            "Craft timers tick faster",
            "+1 stone and gold/tick",
            "Morale never below 50"
        };
        snprintf(buf, sizeof(buf), " ★ HERO: %s — %s",
                 trait_names[d->hero_trait], trait_desc[d->hero_trait]);
        renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DETAIL + 1, COL_GOLD, buf);
    }

    /* Row 2: morale bar + fatigue bar side by side */
    make_bar(bar, d->morale, 100);
    snprintf(buf, sizeof(buf), " Morale:  %s  %3d%%", bar, d->morale);
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DETAIL + 2,
                            morale_color(d->morale), buf);
    make_bar(bar, d->fatigue, 100);
    snprintf(buf, sizeof(buf), " Fatigue: %s  %3d%%", bar, d->fatigue);
    renderer_draw_text_grid(r, UI_COL_MARGIN + 30, UI_ROW_DETAIL + 2,
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
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DETAIL + 3, COL_GOLD, buf);

    /* Row 4: per-job levels */
    static const char *jnames[] = {"Idle","Mine","Lumb","Farm","Guard","Scholar","Craft"};
    int col = UI_COL_MARGIN;
    renderer_draw_text_grid(r, col, UI_ROW_DETAIL + 4, COL_DIM, " Jobs: ");
    col += 7;
    for (int j = 1; j < JOB_COUNT; j++) {
        snprintf(buf, sizeof(buf), "%s Lv%d  ", jnames[j], d->job_level[j]);
        uint32_t jcol = (j == d->job) ? COL_ACCENT : COL_DIM;
        renderer_draw_text_grid(r, col, UI_ROW_DETAIL + 4, jcol, buf);
        col += 12;
    }

    /* Row 4: equipment slot + inventory counts */
    {
        static const char    *equip_names[] = { "None", "Weapon", "Armour", "Tool" };
        static const uint32_t equip_cols[]  = { COL_DIM, 0xFF8888FF, 0xAAAAFFFF, COL_GOLD };
        uint8_t eq = d->equipment;
        uint32_t eq_col = (eq < 4) ? equip_cols[eq] : COL_DIM;
        const char *eq_name = (eq < 4) ? equip_names[eq] : "???";

        /* Left: equipped item */
        snprintf(buf, sizeof(buf), " Equipped: %-8s", eq_name);
        renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DETAIL + 5, eq_col, buf);

        /* Right: equip key hints */
        int ws_lv = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_WORKSHOP);
        if (ws_lv >= 1) {
            renderer_draw_text_grid(r, UI_COL_MARGIN + 19, UI_ROW_DETAIL + 5,
                                    COL_DIM,
                                    "  [1] Weapon  [2] Armour  [3] Tool  [0] Unequip");
        }
    }

    /* Row 6: job assignment keys */
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_DETAIL + 6, COL_DIM,
        " [M]iner [L]umberer [F]armer [G]uard [S]cholar [C]raftsdwarf [I]dle  [E] Feed");
}

/* =========================================================
 * Victory / Win screen
 * Shown when all three World Wonders are complete.
 * ========================================================= */

void ui_draw_win_screen(Renderer *r, const GameState *state) {
    char buf[128];
    int  row = UI_ROW_DWARVES;
    int  ac  = 0;
    for (int i = 0; i < MAX_DWARVES; i++) if (state->dwarves[i].alive) ac++;

    renderer_draw_hline_partial(r, row, 0, DIVIDER_COL, COL_GOLD);
    row++;
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_GOLD,
        "  \xe2\x98\x85  THE MOUNTAIN KING'S LEGACY IS COMPLETE  \xe2\x98\x85");
    row += 2;
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_FG,
        "  Three great wonders stand as testament to the");
    row++;
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_FG,
        "  indomitable spirit and craft of your clan.");
    row += 2;
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_GOLD,
        "  [*] Throne of the Mountain King     COMPLETE");
    row++;
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_GOLD,
        "  [*] World Drill                     COMPLETE");
    row++;
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_GOLD,
        "  [*] Divine Hammer                   COMPLETE");
    row += 2;
    snprintf(buf, sizeof(buf), "  Ticks survived: %llu    Raids repelled: %d",
             (unsigned long long)state->tick, state->raid.raids_completed);
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, buf);
    row++;
    snprintf(buf, sizeof(buf), "  Dwarves sworn: %d       Depths delved: %u / %d",
             ac, state->depth, DEPTH_MAX);
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, buf);
    row += 2;
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_ACCENT,
        "  [F5] Save your victory    [ESC] Continue playing");
    row += 2;
    renderer_draw_hline_partial(r, row, 0, DIVIDER_COL, COL_GOLD);
}

/* =========================================================
 * Help panel  ([/] key)
 * ========================================================= */
void ui_draw_help(Renderer *r) {
    int row = UI_ROW_DWARVES;
    renderer_draw_hline_partial(r, row, 0, DIVIDER_COL, COL_DIM);
    row++;
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_ACCENT,
        "  [ HELP \xe2\x80\x94 KEY BINDINGS ]");
    row += 2;

    int lc = UI_COL_MARGIN;
    int rc = UI_COL_MARGIN + 44;

    renderer_draw_text_grid(r, lc, row, COL_FG,  "  GENERAL");
    renderer_draw_text_grid(r, rc, row, COL_FG,  "  DWARVES");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [H]        Hire dwarf");
    renderer_draw_text_grid(r, rc, row, COL_DIM, "  [M/L/F/G/S/C/I] Assign job");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [U]        Upgrades");
    renderer_draw_text_grid(r, rc, row, COL_DIM, "  [E]   Feed selected dwarf");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [R]        Research");
    renderer_draw_text_grid(r, rc, row, COL_DIM, "  [1/2/3]   Equip weapon/armour/tool");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [P]        Prestige");
    renderer_draw_text_grid(r, rc, row, COL_DIM, "  [0]        Unequip");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [W]        Workshop");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [T]        Tavern");
    renderer_draw_text_grid(r, rc, row, COL_FG,  "  BREACH");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [B]        Breach");
    renderer_draw_text_grid(r, rc, row, COL_DIM, "  [TAB]     Cycle placement mode");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [D]        Dig deeper");
    renderer_draw_text_grid(r, rc, row, COL_DIM, "  [ENTER]   Place defence");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [A]        Achievements");
    renderer_draw_text_grid(r, rc, row, COL_DIM, "  [X]        Remove defence");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [SPACE]    This help");
    renderer_draw_text_grid(r, rc, row, COL_DIM, "  [R]        Retreat");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [ESC/Q]    Close / Quit");
    row += 2;
    renderer_draw_text_grid(r, lc, row, COL_FG,  "  NAVIGATION");
    renderer_draw_text_grid(r, rc, row, COL_FG,  "  SAVE / LOAD");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [↑/↓]      Select dwarf");
    renderer_draw_text_grid(r, rc, row, COL_DIM, "  [F5]  Save");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [←/→]      Scroll list");
    renderer_draw_text_grid(r, rc, row, COL_DIM, "  [F9]  Load");
    row++;
    renderer_draw_text_grid(r, lc, row, COL_DIM, "  [PgUp/Dn]  Scroll event log");
    row += 2;
    renderer_draw_hline_partial(r, row, 0, DIVIDER_COL, COL_DIM);
    row++;
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, "  [SPACE] or [ESC] Close");
}

/* =========================================================
 * Achievement panel  ([A] key)
 * ========================================================= */

static int ach_alive_count(const GameState *state) {
    int n = 0;
    for (int i = 0; i < MAX_DWARVES; i++)
        if (state->dwarves[i].alive) n++;
    return n;
}

static int ach_has_hero(const GameState *state) {
    for (int i = 0; i < MAX_DWARVES; i++)
        if (state->dwarves[i].alive && state->dwarves[i].is_hero) return 1;
    return 0;
}

static int ach_max_job_level(const GameState *state) {
    int best = 0;
    for (int i = 0; i < MAX_DWARVES; i++) {
        if (!state->dwarves[i].alive) continue;
        for (int j = 1; j < JOB_COUNT; j++)
            if (state->dwarves[i].job_level[j] > best)
                best = state->dwarves[i].job_level[j];
    }
    return best;
}

void ui_draw_achievements(Renderer *r, const GameState *state) {
    static const char *names[10] = {
        "The Clan Grows", "Full Barracks",
        "Second Level",   "Heart of Stone",
        "First Blood",    "Unbreachable",
        "The Long Watch", "Iron Clan",
        "Chosen One",     "Master Crafter",
    };
    static const char *descs[10] = {
        "2+ dwarves alive",     "16 dwarves at once",
        "Reach depth 2",        "Reach depth 5",
        "Win your first raid",  "Win 10 raids",
        "2000 ticks survived",  "Hold 10+ iron bars",
        "Hire a hero dwarf",    "Any dwarf at job Lv5",
    };

    int ac = ach_alive_count(state);
    int unlocked[10] = {
        ac >= 2,
        ac >= 16,
        state->depth >= 2,
        state->depth >= 5,
        state->raid.raids_completed >= 1,
        state->raid.raids_completed >= 10,
        state->tick >= 2000,
        state->resources.iron_bars >= 10,
        ach_has_hero(state),
        ach_max_job_level(state) >= MAX_JOB_LEVEL,
    };

    int total = 0;
    for (int i = 0; i < 10; i++) total += unlocked[i];

    int row = UI_ROW_DWARVES;
    renderer_draw_hline_partial(r, row, 0, DIVIDER_COL, COL_DIM);
    row++;

    char buf[80];
    snprintf(buf, sizeof(buf), "  [ ACHIEVEMENTS  %d / 10 ]", total);
    renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                            total == 10 ? COL_GOLD : COL_ACCENT, buf);
    row += 2;

    int lc = UI_COL_MARGIN;
    int rc = UI_COL_MARGIN + 44;

    for (int i = 0; i < 10; i += 2) {
        uint32_t lcol = unlocked[i]   ? COL_GOLD : COL_DIM;
        uint32_t rcol = (i+1 < 10 && unlocked[i+1]) ? COL_GOLD : COL_DIM;

        snprintf(buf, sizeof(buf), "  %s %s",
                 unlocked[i] ? "[*]" : "[ ]", names[i]);
        renderer_draw_text_grid(r, lc, row, lcol, buf);

        if (i + 1 < 10) {
            snprintf(buf, sizeof(buf), "  %s %s",
                     unlocked[i+1] ? "[*]" : "[ ]", names[i+1]);
            renderer_draw_text_grid(r, rc, row, rcol, buf);
        }
        row++;

        snprintf(buf, sizeof(buf), "      %s", descs[i]);
        renderer_draw_text_grid(r, lc, row, COL_DIM, buf);
        if (i + 1 < 10) {
            snprintf(buf, sizeof(buf), "      %s", descs[i+1]);
            renderer_draw_text_grid(r, rc, row, COL_DIM, buf);
        }
        row += 2;
    }

    renderer_draw_hline_partial(r, row, 0, DIVIDER_COL, COL_DIM);
    row++;
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM,
        "  [A] or [ESC] Close");
}

/* =========================================================
 * Depth confirmation popup
 * Shown when player presses D — overlays the main panel.
 * ========================================================= */
void ui_draw_depth_confirm(Renderer *r, const GameState *state) {
    if (!ui_show_depth) return;

    char buf[128];
    uint32_t depth = state->depth;

    /* Already at max depth — nothing to show */
    if (depth >= DEPTH_MAX) {
        renderer_draw_text_grid(r, UI_COL_MARGIN, 8, COL_DIM,
            "  You have reached the deepest halls. No further descent is possible.");
        renderer_draw_text_grid(r, UI_COL_MARGIN, 10, COL_DIM,
            "  [D] or [ESC] Close");
        return;
    }

    /* Per-depth unlock descriptions and costs */
    static const char *unlock_title[] = {
        "", /* depth 1 → 2 */
        "Iron Ore deposits open up — smelt bars, forge weapons and armour.",
        "Gem seams discovered — enables traps, tools, and rune research.",
        "Ancient Relics surface — accelerates prestige gain.",
        "Crystal formations exposed — endgame mana and conduit upgrades."
    };
    static const int cost_stone[] = { 0, 500, 1500, 3000, 6000 };
    static const int cost_gold[]  = { 0, 300, 1000, 2500, 5000 };

    int next = (int)depth; /* index into cost/unlock arrays (depth 1→2 uses [1]) */

    int stone_ok = state->resources.stone >= cost_stone[next];
    int gold_ok  = state->resources.gold  >= cost_gold[next];
    int can_dig  = stone_ok && gold_ok;

    /* Box — draw over rows 6..18 of the left panel */
    int row = 6;
    renderer_draw_hline_partial(r, row, 0, DIVIDER_COL, COL_DIM);
    row++;

    snprintf(buf, sizeof(buf), "  [ DESCEND TO DEPTH %u / %d ]", depth + 1, DEPTH_MAX);
    renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                            can_dig ? COL_ACCENT : COL_GOLD, buf);
    row += 2;

    /* What unlocks */
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_FG, "  What awaits:");
    row++;
    snprintf(buf, sizeof(buf), "    %s", unlock_title[next]);
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, buf);
    row += 2;

    /* Cost lines */
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_FG, "  Cost:");
    row++;

    snprintf(buf, sizeof(buf), "    Stone: %d  (have %lld)",
             cost_stone[next], (long long)state->resources.stone);
    renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                            stone_ok ? COL_FOOD : 0xFF4444FF, buf);
    row++;

    snprintf(buf, sizeof(buf), "    Gold:  %d  (have %lld)",
             cost_gold[next], (long long)state->resources.gold);
    renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                            gold_ok ? COL_FOOD : 0xFF4444FF, buf);
    row += 2;

    /* Confirm / cancel */
    if (can_dig) {
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_ACCENT,
            "  [D] Confirm descent   [ESC] Cancel");
    } else {
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, 0xFF4444FF,
            "  Not enough resources.");
        row++;
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM,
            "  [ESC] Close");
    }

    row += 2;
    renderer_draw_hline_partial(r, row, 0, DIVIDER_COL, COL_DIM);
}

/* =========================================================
 * Command bar
 * ========================================================= */
void ui_draw_cmdbar(Renderer *r, const GameState *state) {
    renderer_draw_hline_partial(r, UI_ROW_CMDBAR - 1, 0, DIVIDER_COL, COL_DIM);

    /* ---- shared calculations ---- */
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
    int ws_lv = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_WORKSHOP);
    int tv_lv = (int)UPGR_LEVEL(state->upgrades.tier1, UPGR_TAVERN);

    /* ---- Row 1: core gameplay keys (stays within DIVIDER_COL) ---- */
    char row1[192];
    const char *breach_hint =
        state->raid.active == RAID_COMBAT  ? "[B] BREACH(!)" :
        state->raid.active == RAID_WARNING ? "[B] Breach(warn)" : "[B] Breach";
    snprintf(row1, sizeof(row1),
             "[H] Hire (%dg/%df)%s  [U] Upgrades  [R] Research  [P] Prestige  %s%s%s",
             hire_gold, hire_food,
             can_hire ? "" : "(!)",
             breach_hint,
             ws_lv >= 1 ? "  [W] Workshop" : "",
             tv_lv >= 1 ? "  [T] Tavern"   : "");
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_CMDBAR,
                            can_hire ? COL_FG : COL_DIM, row1);

    /* ---- Row 2: panels and meta keys ---- */
    char row2[192];
    snprintf(row2, sizeof(row2),
             "[D] Depth  [J] Wonders  [A] Achievements  [SPACE] Help  [F5] Save  [F9] Load");
    renderer_draw_text_grid(r, UI_COL_MARGIN, UI_ROW_CMDBAR + 1, COL_DIM, row2);
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
