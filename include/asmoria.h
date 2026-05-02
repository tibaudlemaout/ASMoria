#ifndef ASMORIA_H
#define ASMORIA_H

#include <stdint.h>

typedef struct {
    int64_t gold;
    int64_t stone;
    int64_t wood;
    int64_t food;
    int64_t mana;
} Resources;

#define MAX_DWARVES 64

typedef struct {
    uint8_t  alive;
    uint8_t  job;
    uint8_t  morale;
    uint8_t  fatigue;
    uint8_t  prev_job;
    uint8_t  _pad[3];
    int64_t  xp;
} Dwarf;

#define JOB_IDLE     0
#define JOB_MINER    1
#define JOB_LUMBERER 2
#define JOB_FARMER   3
#define JOB_GUARD    4
#define JOB_SCHOLAR  5

/* Base hire costs — reduced by Recruiters upgrade */
#define HIRE_GOLD_BASE  50
#define HIRE_FOOD_BASE  20

/* Keep old names as aliases for UI */
#define HIRE_GOLD_COST  HIRE_GOLD_BASE
#define HIRE_FOOD_COST  HIRE_FOOD_BASE

/* =========================================================
 * Upgrade system
 *
 * Levels packed as 4-bit nibbles in Upgrades.tier1:
 *   bits [0..3]  = PICK_QUALITY  (tools,     max 3)
 *   bits [4..7]  = SAW_QUALITY   (tools,     max 3)
 *   bits [8..11] = IRRIGATION    (tools,     max 3)
 *   bits [12..15]= BARRACKS      (workforce, max 3)
 *   bits [16..19]= RECRUITERS    (workforce, max 3)
 *
 * Cost for next level:
 *   Tools:     gold = 100 * next,  stone = 50 * next
 *   Workforce: gold = 150 * next,  stone = 75 * next
 * ========================================================= */
#define UPGR_PICK_QUALITY   0
#define UPGR_SAW_QUALITY    1
#define UPGR_IRRIGATION     2
#define UPGR_BARRACKS       3
#define UPGR_RECRUITERS     4
#define UPGR_COUNT          5

#define UPGR_MAX_TOOLS      3
#define UPGR_MAX_WORKFORCE  3

/* Extract level of upgrade id from tier1 */
#define UPGR_LEVEL(tier1, id)  (((tier1) >> ((id) * 4)) & 0xF)

/* Starting dwarf cap (Barracks Lv0) */
#define DWARF_CAP_BASE      16
#define DWARF_CAP_PER_LEVEL 16

/* Hire cost reduction per Recruiters level */
#define HIRE_GOLD_DISCOUNT  10
#define HIRE_FOOD_DISCOUNT   5

/* Cost bases */
#define UPGR_COST_GOLD_TOOLS    100
#define UPGR_COST_STONE_TOOLS    50
#define UPGR_COST_GOLD_WORK     150
#define UPGR_COST_STONE_WORK     75

typedef struct {
    uint64_t tier1;
    uint64_t tier2;
} Upgrades;

typedef struct { uint64_t seed; } RngState;

typedef struct {
    uint8_t  code;
    uint8_t  severity;
    uint8_t  dwarf_idx;
    uint8_t  _pad[5];
} PendingEvent;

#define EVT_FLAVOUR     0
#define EVT_POSITIVE    1
#define EVT_NEGATIVE    2
#define EVT_MILESTONE   3
#define EVENT_LOG_SIZE  64
#define EVENT_INTERVAL  30

typedef struct {
    uint64_t tick;
    uint8_t  code;
    uint8_t  severity;
    uint8_t  dwarf_idx;
    uint8_t  _pad[5];
} EventRecord;

typedef struct {
    EventRecord entries[EVENT_LOG_SIZE];
    uint8_t     head;
    uint8_t     count;
    uint8_t     _pad[6];
} EventLog;

typedef struct {
    /* offset 0x0000 */ Resources    resources;
    /* offset 0x0028 */ Dwarf        dwarves[MAX_DWARVES];
    /* offset 0x0428 */ Upgrades     upgrades;
    /* offset 0x0438 */ RngState     rng;
    /* offset 0x0440 */ uint64_t     tick;
    /* offset 0x0448 */ uint32_t     depth;
    /* offset 0x044C */ uint32_t     flags;
    /* offset 0x0450 */ PendingEvent pending;
    /* offset 0x0458 */ EventLog     event_log;
} GameState;

extern void     asm_tick(GameState *state);
extern void     asm_rng_seed(GameState *state, uint64_t seed);
extern uint64_t asm_rng_next(GameState *state);
extern void     asm_event_push(GameState *state, uint8_t code,
                               uint8_t severity, uint8_t dwarf_idx);
extern void     asm_tick_dwarves(GameState *state);
extern int64_t  asm_hire_dwarf(GameState *state);
extern int64_t  asm_assign_job(GameState *state, uint8_t dwarf_idx,
                               uint8_t job);
extern int64_t  asm_buy_upgrade(GameState *state, uint8_t upgrade_id);

#endif /* ASMORIA_H */