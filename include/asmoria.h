#ifndef ASMORIA_H
#define ASMORIA_H

#include <stdint.h>

/* =========================================================
 * ASMoria - Shared State & ASM API
 *
 * This header is the single source of truth for the
 * GameState memory layout. The ASM modules reference
 * fields via hardcoded offsets that must stay in sync
 * with this struct. If you add/reorder fields, update
 * asm/core/offsets.inc accordingly.
 * ========================================================= */

/* --- Resource block (offset 0x00) --- */
typedef struct {
    int64_t gold;           /* +0x00 */
    int64_t stone;          /* +0x08 */
    int64_t wood;           /* +0x10 */
    int64_t food;           /* +0x18 */
    int64_t mana;           /* +0x20 */
} Resources;                /* size: 0x28 */

/* --- Dwarf block (offset 0x28 in GameState) --- */
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

/* --- Master game state --- */
typedef struct {
    /* offset 0x0000 */ Resources  resources;
    /* offset 0x0028 */ Dwarf      dwarves[MAX_DWARVES];
    /* offset 0x0828 */ Upgrades   upgrades;
    /* offset 0x0838 */ RngState   rng;
    /* offset 0x0840 */ uint64_t   tick;       /* total ticks elapsed  */
    /* offset 0x0848 */ uint32_t   depth;      /* mine depth reached   */
    /* offset 0x084C */ uint32_t   flags;      /* misc game flags      */
} GameState;

/* =========================================================
 * ASM-exported function declarations
 * Defined in asm/core - tick.asm, resources.asm, rng.asm
 * ========================================================= */

/* Advance game state by one tick */
extern void asm_tick(GameState *state);

/* RNG: seed the xorshift64 generator */
extern void asm_rng_seed(GameState *state, uint64_t seed);

/* RNG: return next pseudo-random uint64 */
extern uint64_t asm_rng_next(GameState *state);

#endif /* ASMORIA_H */