#include <SFML/Graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "idle_core.h"
#include "dwarves.h"
#include "resources.h"

#include <SFML/Graphics.h>
#include <stdio.h>

#include "idle_core.h"
#include "dwarves.h"
#include "resources.h"

static void update_text(sfText *text, const char *label, uint64_t value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s: %llu", label, value);
    sfText_setString(text, buffer);
}

int main(void) {
    sfVideoMode mode = {800, 600, 32};
    sfRenderWindow *window =
        sfRenderWindow_create(mode, "ASMoria", sfResize | sfClose, NULL);

    if (!window)
        return 1;

    init_resources();
    add_dwarf(RES_STONE); // test dwarf

    /* ---------- Font ---------- */
    sfFont *font = sfFont_createFromFile("assets/font.ttf");
    if (!font)
        return 1;

    /* ---------- Text objects ---------- */
    sfText *gold_text  = sfText_create();
    sfText *stone_text = sfText_create();
    sfText *iron_text  = sfText_create();

    sfText_setFont(gold_text, font);
    sfText_setFont(stone_text, font);
    sfText_setFont(iron_text, font);

    sfText_setCharacterSize(gold_text, 24);
    sfText_setCharacterSize(stone_text, 24);
    sfText_setCharacterSize(iron_text, 24);

    sfText_setPosition(gold_text,  (sfVector2f){20, 20});
    sfText_setPosition(stone_text, (sfVector2f){20, 60});
    sfText_setPosition(iron_text,  (sfVector2f){20, 100});

    while (sfRenderWindow_isOpen(window)) {
        sfEvent event;
        while (sfRenderWindow_pollEvent(window, &event)) {
            if (event.type == sfEvtClosed)
                sfRenderWindow_close(window);
        }

        /* ---------- Game logic ---------- */
        game_tick();
        dwarf_tick();

        /* ---------- Update UI ---------- */
        update_text(gold_text,  "Gold",  get_gold());
        update_text(stone_text, "Stone", get_stone());
        update_text(iron_text,  "Iron",  get_iron());

        /* ---------- Render ---------- */
        sfRenderWindow_clear(window, sfColor_fromRGB(25, 25, 25));
        sfRenderWindow_drawText(window, gold_text, NULL);
        sfRenderWindow_drawText(window, stone_text, NULL);
        sfRenderWindow_drawText(window, iron_text, NULL);
        sfRenderWindow_display(window);
    }

    /* ---------- Cleanup ---------- */
    sfText_destroy(gold_text);
    sfText_destroy(stone_text);
    sfText_destroy(iron_text);
    sfFont_destroy(font);
    sfRenderWindow_destroy(window);
    return 0;
}
