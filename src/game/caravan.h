#ifndef CARAVAN_H
#define CARAVAN_H

#include "../../include/asmoria.h"

/* Phase constants — mirror CAVAN_* in offsets.inc */
#define CARAVAN_IDLE             CAVAN_IDLE
#define CARAVAN_INFLIGHT         CAVAN_INFLIGHT
#define CARAVAN_ARRIVED_SUCCESS  CAVAN_SUCCESS
#define CARAVAN_ARRIVED_FAIL     CAVAN_FAIL

#define CARAVAN_MAX_WORKERS  8
#define CARAVAN_MAX_GUARDS   6

/* Convenience: failure % given guard count (used by UI) */
static inline int caravan_fail_pct(int guards) {
    int p = 60 - guards * 10;
    return p < 5 ? 5 : p;
}

/* Assembly-implemented functions */
extern void asm_caravan_launch(GameState *state, int workers,
                               int guards, int64_t food);
extern void asm_tick_caravan(GameState *state);
extern void asm_caravan_acknowledge(GameState *state);

#endif