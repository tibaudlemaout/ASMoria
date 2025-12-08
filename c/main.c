#include <stdio.h>
#include <unistd.h>
#include "idle_core.h"
#include "dwarves.h"
#include "resources.h"

int main() {
    init_resources();

    // Example: give 1 dwarf assigned to stone
    add_dwarf(RES_STONE);

    while (1) {
        asm_tick();   // gold += 1
        dwarf_tick(); // stone += dwarves assigned to stone

        if (get_gold() > 5) {
            move_dwarf(0, RES_IRON); // Move dwarf to iron
        }

        printf("Gold: %lu, Stone: %lu, Iron: %lu\n",
            get_gold(), get_stone(), get_iron());
        sleep(1);
    }
}
