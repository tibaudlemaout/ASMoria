#include <SFML/Graphics.h>
#include <stdio.h>
#include "idle_core.h"
#include "dwarves.h"
#include "resources.h"

int main(void) {
    sfVideoMode mode = {800, 600, 32};
    sfRenderWindow *window =
        sfRenderWindow_create(mode, "ASMoria", sfResize | sfClose, NULL);

    if (!window)
        return 1;

    init_resources();

    // Example: start with one dwarf on stone
    add_dwarf(RES_STONE);

    sfClock *clock = sfClock_create();

    while (sfRenderWindow_isOpen(window)) {
        sfEvent event;

        while (sfRenderWindow_pollEvent(window, &event)) {
            if (event.type == sfEvtClosed)
                sfRenderWindow_close(window);
        }

        // Tick every frame (later we’ll control speed)
        game_tick();
        dwarf_tick();

        // Clear window (dark gray)
        sfRenderWindow_clear(window, sfColor_fromRGB(30, 30, 30));

        // (Rendering will go here later)

        sfRenderWindow_display(window);
    }

    sfClock_destroy(clock);
    sfRenderWindow_destroy(window);
    return 0;
}
