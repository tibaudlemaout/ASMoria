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

/* Hire cost constants (keep in sync with hire.asm) */
#define HIRE_GOLD_COST  50
#define HIRE_FOOD_COST  20

typedef struct {
    uint64_t tier1;
    uint64_t tier2;
} Upgrades;

typedef struct {
    uint64_t seed;
} RngState;

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

/* Returns hired dwarf index (0-63), or -1 if failed */
extern int64_t  asm_hire_dwarf(GameState *state);

/* Returns 1 on success, 0 if dwarf is dead/resting/invalid */
extern int64_t  asm_assign_job(GameState *state, uint8_t dwarf_idx,
                               uint8_t job);

#endif /* ASMORIA_H */