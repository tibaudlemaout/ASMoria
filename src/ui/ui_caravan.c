#include "ui_caravan.h"
#include "ui.h"
#include "../game/caravan.h"
#include <stdio.h>

int ui_show_caravan = 0;

/* UI-side configuration (pre-launch inputs) */
static int     cfg_field   = 0;    /* 0=workers  1=guards  2=food */
static int     cfg_workers = 1;
static int     cfg_guards  = 0;
static int64_t cfg_food    = 100;

int     ui_caravan_cur_workers(void) { return cfg_workers; }
int     ui_caravan_cur_guards(void)  { return cfg_guards;  }
int64_t ui_caravan_cur_food(void)    { return cfg_food;    }

void ui_caravan_field_move(int delta) {
    cfg_field = (cfg_field + delta + 3) % 3;
}

void ui_caravan_adjust(int delta) {
    switch (cfg_field) {
        case 0:
            cfg_workers += delta;
            if (cfg_workers < 1)                   cfg_workers = 1;
            if (cfg_workers > CARAVAN_MAX_WORKERS)  cfg_workers = CARAVAN_MAX_WORKERS;
            break;
        case 1:
            cfg_guards += delta;
            if (cfg_guards < 0)                    cfg_guards = 0;
            if (cfg_guards > CARAVAN_MAX_GUARDS)   cfg_guards = CARAVAN_MAX_GUARDS;
            break;
        case 2:
            cfg_food += delta * CARAVAN_FOOD_STEP;
            if (cfg_food < CARAVAN_FOOD_STEP)  cfg_food = CARAVAN_FOOD_STEP;
            if (cfg_food > CARAVAN_FOOD_MAX)   cfg_food = CARAVAN_FOOD_MAX;
            break;
    }
}

/* =========================================================
 * Draw
 * ========================================================= */
void ui_draw_caravan(Renderer *r, const GameState *state) {
    char     buf[128];
    int      row = UI_ROW_DWARVES;

    /* Unpack live state from the assembly-managed packed uint32 */
    uint32_t cs    = state->raid.caravan_state;
    int      phase = (int)CAVAN_PHASE(cs);

    renderer_draw_hline_partial(r, row, 0, DIVIDER_COL, COL_DIM);
    row++;

    /* Header */
    const char *phase_str =
        phase == CAVAN_INFLIGHT ? "IN FLIGHT"  :
        phase == CAVAN_SUCCESS  ? "RETURNED"   :
        phase == CAVAN_FAIL     ? "AMBUSHED!"  : "SEND A CARAVAN";
    uint32_t hcol =
        phase == CAVAN_FAIL    ? 0xFF4444FF :
        phase == CAVAN_SUCCESS ? COL_FOOD   : COL_ACCENT;
    snprintf(buf, sizeof(buf), "  [ CARAVAN -- %s ]", phase_str);
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, hcol, buf);
    row += 2;

    /* ---- In-flight view ---- */
    if (phase == CAVAN_INFLIGHT) {
        snprintf(buf, sizeof(buf),
                 "  Workers: %d    Guards: %d    Food sent: %lld",
                 (int)CAVAN_WORKERS(cs), (int)CAVAN_GUARDS(cs),
                 (long long)CAVAN_FOOD_H(cs) * CARAVAN_FOOD_STEP);
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_FG, buf);
        row++;
        snprintf(buf, sizeof(buf), "  Arriving in %d ticks...",
                 (int)CAVAN_TIMER(cs));
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, buf);
        row += 2;
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, "  [K/ESC] Close");
        return;
    }

    /* ---- Result view ---- */
    if (phase == CAVAN_SUCCESS || phase == CAVAN_FAIL) {
        if (phase == CAVAN_SUCCESS) {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_FOOD,
                "  Your traders returned safely.");
            row++;
            if (CAVAN_BP_FOUND(cs)) {
                static const char *wnames[3] = {
                    "Throne of the Mountain King",
                    "World Drill",
                    "Divine Hammer"
                };
                snprintf(buf, sizeof(buf), "  Blueprint obtained: %s!",
                         wnames[CAVAN_BP_IDX(cs)]);
                renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_GOLD, buf);
            } else {
                renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM,
                    "  No blueprint found this trip. Try sending more food.");
            }
        } else {
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, 0xFF4444FF,
                "  The caravan was ambushed on the road!");
            row++;
            snprintf(buf, sizeof(buf),
                     "  %d dwarves returned exhausted and broken.",
                     (int)CAVAN_WORKERS(cs) + (int)CAVAN_GUARDS(cs));
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, 0xFF4444FF, buf);
        }
        row += 2;
        renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_ACCENT,
            "  [ENTER] Acknowledge    [K/ESC] Close");
        return;
    }

    /* ---- Idle: configuration UI ---- */
    int64_t food_have = state->resources.food;
    int     fail_pct  = caravan_fail_pct(cfg_guards);
    int     can_send  = (cfg_workers >= 1 && food_have >= cfg_food);

    /* Three input fields */
    static const char *flabels[3] = { "Workers", "Guards", "Food" };
    int fvals[3] = { cfg_workers, cfg_guards, (int)cfg_food };
    for (int f = 0; f < 3; f++) {
        int lc        = UI_COL_MARGIN + f * 28;
        uint32_t fc   = (f == cfg_field) ? COL_ACCENT : COL_FG;
        const char *sel = (f == cfg_field) ? "> " : "  ";
        if (f == 2)
            snprintf(buf, sizeof(buf), "%s%s: %d%s",
                     sel, flabels[f], fvals[f],
                     food_have < cfg_food ? "(!)" : "");
        else
            snprintf(buf, sizeof(buf), "%s%s: %d", sel, flabels[f], fvals[f]);
        renderer_draw_text_grid(r, lc, row, fc, buf);
    }
    row++;
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM,
        "    [</>) switch field    [^/v] adjust value");
    row += 2;

    /* Risk */
    snprintf(buf, sizeof(buf),
             "  Failure risk: %d%%   (base 60%% - %d guards x 10%%, min 5%%)",
             fail_pct, cfg_guards);
    uint32_t rcol = fail_pct >= 50 ? 0xFF4444FF :
                    fail_pct >= 20 ? COL_GOLD   : COL_FOOD;
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, rcol, buf);
    row++;

    int bonus = (int)(cfg_food / CARAVAN_FOOD_STEP);
    snprintf(buf, sizeof(buf),
             "  Blueprint bonus: +%d%% per wonder  (%lld food / 100)",
             bonus, (long long)cfg_food);
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_DIM, buf);
    row += 2;

    /* Blueprint chances */
    renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_FG, "  Wonder blueprints:");
    row++;
    static const char *wnames[3] = {
        "Throne of the Mountain King",
        "World Drill",
        "Divine Hammer"
    };
    static const int base_pct[3] = { 30, 20, 10 };
    for (int w = 0; w < WONDER_COUNT; w++) {
        if (WONDER_HAS_BP(state->upgrades.tier2, w)) {
            snprintf(buf, sizeof(buf), "    %-30s [HAVE BLUEPRINT]", wnames[w]);
            renderer_draw_text_grid(r, UI_COL_MARGIN, row, COL_GOLD, buf);
        } else {
            int chance = base_pct[w] + bonus;
            if (chance > 95) chance = 95;
            snprintf(buf, sizeof(buf), "    %-30s %d%% chance", wnames[w], chance);
            renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                                    chance >= 20 ? COL_FOOD : COL_DIM, buf);
        }
        row++;
    }
    row++;

    renderer_draw_text_grid(r, UI_COL_MARGIN, row,
                            can_send ? COL_ACCENT : COL_DIM,
                            can_send
                              ? "  [ENTER] Send caravan    [K/ESC] Close"
                              : "  [need more food]        [K/ESC] Close");
}
