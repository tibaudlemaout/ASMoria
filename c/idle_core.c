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

void set_gold_rate(uint64_t rate) {
    gold_rate = rate;
}

void set_stone_rate(uint64_t rate) {
    stone_rate = rate;
}

void set_iron_rate(uint64_t rate) {
    iron_rate = rate;
}

// Combined tick function
void game_tick(void) {
    asm_tick();        // gold
    resource_tick();   // stone & iron
}
