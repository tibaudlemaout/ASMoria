#ifndef ASMORIA_H
#define ASMORIA_H

#include <stdint.h>

typedef struct {
    int64_t gold, stone, wood, food, mana;
} Resources;

#define MAX_DWARVES     64
#define JOB_COUNT       6       /* idle=0 miner=1 lumberer=2 farmer=3 guard=4 scholar=5 */
#define MAX_JOB_LEVEL   5

typedef struct {
    uint8_t  alive;             /* +0x00 */
    uint8_t  job;               /* +0x01 */
    uint8_t  morale;            /* +0x02 */
    uint8_t  fatigue;           /* +0x03 */
    uint8_t  prev_job;          /* +0x04 */
    uint8_t  job_level[6];      /* +0x05 : level per job (indexed by JOB_*) */
    uint8_t  name_idx;          /* +0x0B : index into dwarf name table */
    int32_t  _pad2;             /* +0x0C */
    int64_t  job_xp[6];         /* +0x10 : xp per job */
} Dwarf;                        /* size: 64 bytes */

#define JOB_IDLE     0
#define JOB_MINER    1
#define JOB_LUMBERER 2
#define JOB_FARMER   3
#define JOB_GUARD    4
#define JOB_SCHOLAR  5

/* XP thresholds for each level (index = level, value = xp needed) */
#define XP_LVL1     0
#define XP_LVL2     500
#define XP_LVL3     1500
#define XP_LVL4     3500
#define XP_LVL5     7500

/* Hire costs */
#define HIRE_GOLD_BASE  50
#define HIRE_FOOD_BASE  20
#define HIRE_GOLD_COST  HIRE_GOLD_BASE
#define HIRE_FOOD_COST  HIRE_FOOD_BASE

/* =========================================================
 * Upgrade system
 * ========================================================= */
#define UPGR_PICK_QUALITY   0
#define UPGR_SAW_QUALITY    1
#define UPGR_IRRIGATION     2
#define UPGR_BARRACKS       3
#define UPGR_RECRUITERS     4
#define UPGR_COUNT          5
#define UPGR_MAX_TOOLS      3
#define UPGR_MAX_WORKFORCE  3
#define UPGR_LEVEL(tier1, id)  (((tier1) >> ((id) * 4)) & 0xF)
#define UPGR_COST_GOLD_TOOLS    100
#define UPGR_COST_STONE_TOOLS    50
#define UPGR_COST_GOLD_WORK     150
#define UPGR_COST_STONE_WORK     75
#define DWARF_CAP_BASE      16
#define DWARF_CAP_PER_LEVEL 16
#define HIRE_GOLD_DISCOUNT  10
#define HIRE_FOOD_DISCOUNT   5

typedef struct { uint64_t tier1, tier2; } Upgrades;
typedef struct { uint64_t seed; } RngState;

typedef struct {
    uint8_t code, severity, dwarf_idx;
    uint8_t _pad[5];
} PendingEvent;

#define EVT_FLAVOUR     0
#define EVT_POSITIVE    1
#define EVT_NEGATIVE    2
#define EVT_MILESTONE   3
#define EVENT_LOG_SIZE  64
#define EVENT_INTERVAL  30

typedef struct {
    uint64_t tick;
    uint8_t  code, severity, dwarf_idx;
    uint8_t  _pad[5];
} EventRecord;

typedef struct {
    EventRecord entries[EVENT_LOG_SIZE];
    uint8_t     head, count;
    uint8_t     _pad[6];
} EventLog;

typedef struct {
    /* offset 0x0000 */ Resources    resources;
    /* offset 0x0028 */ Dwarf        dwarves[MAX_DWARVES];
    /* offset 0x1028 */ Upgrades     upgrades;
    /* offset 0x1038 */ RngState     rng;
    /* offset 0x1040 */ uint64_t     tick;
    /* offset 0x1048 */ uint32_t     depth;
    /* offset 0x104C */ uint32_t     flags;
    /* offset 0x1050 */ PendingEvent pending;
    /* offset 0x1058 */ EventLog     event_log;
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
extern const char *asm_get_dwarf_name(uint8_t idx);
extern int64_t  asm_buy_upgrade(GameState *state, uint8_t upgrade_id);
extern uint64_t asm_state_checksum(const void *data, uint64_t len);
extern int64_t  asm_save_game(const char *path, const GameState *state);
extern int64_t  asm_load_game(const char *path, GameState *state);

#endif /* ASMORIA_H */