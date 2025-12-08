#include "idle_core.h"
#include <stdint.h>

// Declare ASM global variables
extern uint64_t gold;

// Return current gold value
uint64_t get_gold(void) {
    return gold;
}
