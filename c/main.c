#include <stdio.h>
#include <unistd.h>
#include "idle_core.h"

int main() {
    init_resources();

    //upgrade production after 5 ticks
    for (int i = 0; i < 10; i++) {
        if (i == 5) {
            set_gold_rate(5);   // upgrade gold production
            set_stone_rate(3);
            set_iron_rate(2);
            printf("Upgrade applied!\n");
        }

        game_tick();
        printf("Tick %d: Gold = %lu, Stone = %lu, Iron = %lu\n",
               i+1, get_gold(), get_stone(), get_iron());
        sleep(1);
    }
}
