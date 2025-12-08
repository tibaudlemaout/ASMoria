#include "idle_core.h"
#include <stdint.h>

// Declare ASM global variables
extern uint64_t gold;

// Return current gold value
uint64_t get_gold(void) {
    return gold;
}

uint64_t get_stone(void) {
    return stone;
}

uint64_t get_iron(void) {
    return iron;
}
