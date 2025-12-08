#include <stdio.h>
#include <unistd.h>      // For sleep()
#include "idle_core.h"

int main() {
    printf("ASMoria Idle Game Test\n");

    // Initialize resources
    init_resources();

    // Run 10 ticks
    for(int i = 0; i < 10; i++) {
        asm_tick();   // Call ASM tick
        resource_tick(); // Increment other resources
        printf("Tick %d: Gold = %lu, Stone = %lu, Iron = %lu\n",
               i+1, get_gold(), get_stone(), get_iron());
        sleep(1);
    }

    printf("Test complete!\n");
    return 0;
}
