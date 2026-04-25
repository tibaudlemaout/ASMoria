#ifndef ASMORIA_H
#define ASMORIA_H

#include <stdint.h>

/* =========================================================
 * ASMoria - Shared State & ASM API
 *
 * This header is the single source of truth for the
 * GameState memory layout. The ASM modules reference
 * fields via hardcoded offsets that must stay in sync
 * with asm/core/offsets.inc. If you add/reorder fields,
 * update offsets.inc accordingly.
 * ========================================================= */

/* --- Resource block (offset 0x00) --- */
typedef struct {
    int64_t gold;           /* +0x00 */
    int64_t stone;          /* +0x08 */
    int64_t wood;           /* +0x10 */
    int64_t food;           /* +0x18 */
    int64_t mana;           /* +0x20 */
} Resources;                /* size: 0x28 */

/* --- Dwarf block --- */
#define MAX_DWARVES 64

typedef struct {
    uint8_t  alive;         /* +0x00 : 1 = alive, 0 = dead    */
    uint8_t  job;           /* +0x01 : see JOB_* constants     */
    uint8_t  morale;        /* +0x02 : 0-100                   */
    uint8_t  fatigue;       /* +0x03 : 0-100                   */
    uint32_t _pad;          /* +0x04 : alignment padding       */
    int64_t  xp;            /* +0x08 : experience points       */
} Dwarf;                    /* size: 0x10 = 16 bytes           */

/* Dwarf job constants */
#define JOB_IDLE     0
#define JOB_MINER    1
#define JOB_LUMBERER 2
#define JOB_FARMER   3
#define JOB_GUARD    4
#define JOB_SCHOLAR  5

/* --- Upgrade flags (bitmask) --- */
typedef struct {
    uint64_t tier1;         /* +0x00 : first 64 upgrades  */
    uint64_t tier2;         /* +0x08 : next 64 upgrades   */
} Upgrades;                 /* size: 0x10                 */

/* --- RNG state (xorshift64) --- */
typedef struct {
    uint64_t seed;          /* +0x00 */
} RngState;

/* --- Event system --- */

/* Severity / colour hint for the renderer */
#define EVT_FLAVOUR     0   /* neutral flavour text    -> parchment */
#define EVT_POSITIVE    1   /* good stat impact        -> green     */
#define EVT_NEGATIVE    2   /* bad stat impact         -> red       */
#define EVT_MILESTONE   3   /* depth/upgrade milestone -> accent    */

/* How many log lines we keep in the ring buffer */
#define EVENT_LOG_SIZE  64

/* An event record written by ASM, read by the UI */
typedef struct {
    uint64_t tick;          /* +0x00 : tick the event fired on      */
    uint8_t  code;          /* +0x08 : message template index       */
    uint8_t  severity;      /* +0x09 : EVT_* constant               */
    uint8_t  dwarf_idx;     /* +0x0A : which dwarf (0xFF = none)    */
    uint8_t  _pad[5];       /* +0x0B : padding to 16 bytes          */
} EventRecord;              /* size : 0x10 = 16 bytes               */

/* Ring buffer of recent events, managed by ASM */
typedef struct {
    EventRecord entries[EVENT_LOG_SIZE]; /* +0x000 : ring buffer     */
    uint8_t     head;                    /* +0x400 : next write slot */
    uint8_t     count;                   /* +0x401 : filled entries  */
    uint8_t     _pad[6];                 /* +0x402 : alignment       */
} EventLog;                              /* size  : 0x408            */

/* Ticks between random flavour events */
#define EVENT_INTERVAL  30

/* --- Master game state --- */
typedef struct {
    /* offset 0x0000 */ Resources  resources;
    /* offset 0x0028 */ Dwarf      dwarves[MAX_DWARVES];
    /* offset 0x0828 */ Upgrades   upgrades;
    /* offset 0x0838 */ RngState   rng;
    /* offset 0x0840 */ uint64_t   tick;
    /* offset 0x0848 */ uint32_t   depth;
    /* offset 0x084C */ uint32_t   flags;
    /* offset 0x0850 */ EventLog   event_log;
} GameState;

/* =========================================================
 * ASM-exported function declarations
 * ========================================================= */

extern void     asm_tick(GameState *state);
extern void     asm_rng_seed(GameState *state, uint64_t seed);
extern uint64_t asm_rng_next(GameState *state);
extern void     asm_event_push(GameState *state, uint8_t code,
                               uint8_t severity, uint8_t dwarf_idx);

#endif /* ASMORIA_H */