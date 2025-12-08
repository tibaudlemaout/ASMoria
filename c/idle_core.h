#ifndef IDLE_CORE_H
#define IDLE_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ASM-exposed functions
void asm_tick(void);       // Perform one tick (increments gold)
void init_resources(void); // Initialize additional resources
void resource_tick(void);   // tick function for other resources


// Accessors
extern uint64_t gold;
extern uint64_t stone;
extern uint64_t iron;
extern uint64_t gold_rate;
extern uint64_t stone_rate;
extern uint64_t iron_rate;

// Getters for current resource values
uint64_t get_gold(void);
uint64_t get_stone(void);
uint64_t get_iron(void);

// Setters for production rates
void set_gold_rate(uint64_t rate);
void set_stone_rate(uint64_t rate);
void set_iron_rate(uint64_t rate);

// Combined tick function
void game_tick(void);

#ifdef __cplusplus
}
#endif

#endif
