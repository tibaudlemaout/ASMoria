#ifndef IDLE_CORE_H
#define IDLE_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ASM-exposed functions
void asm_tick(void);       // Perform one tick (increments gold)
void init_resources(void); // Initialize additional resources

// Accessors
uint64_t get_gold(void);

#ifdef __cplusplus
}
#endif

#endif
