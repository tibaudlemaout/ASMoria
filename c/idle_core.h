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
// Accessors
extern uint64_t gold;
extern uint64_t stone;
extern uint64_t iron;

uint64_t get_gold(void);
uint64_t get_stone(void);
uint64_t get_iron(void);

#ifdef __cplusplus
}
#endif

#endif
