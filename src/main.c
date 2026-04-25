#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>

#include "../include/asmoria.h"
#include "render/renderer.h"
#include "ui/ui.h"
#include "game/game.h"

/* =========================================================
 * ASMoria - Entry Point
 *
 * Main loop structure:
 *   - Poll SDL events (quit, keyboard)
 *   - On TICK_MS elapsed: call game_update() [-> ASM]
 *   - Every frame: render the current state
 * ========================================================= */

int main(void) {
    Renderer  renderer;
    GameState state;

    /* --- Init --- */
    if (renderer_init(&renderer, "ASMoria") != 0)
        return 1;

    game_init(&state);

    /* --- Game loop --- */
    int      running       = 1;
    uint64_t last_tick_ms  = SDL_GetTicks64();

    while (running) {
        /* Event handling */
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    running = 0;
                    break;

                case SDL_KEYDOWN:
                    switch (ev.key.keysym.sym) {
                        case SDLK_ESCAPE:
                        case SDLK_q:
                            running = 0;
                            break;
                        /* (future key bindings go here) */
                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }
        }

        /* Tick logic — decouple game speed from render speed */
        uint64_t now = SDL_GetTicks64();
        if (now - last_tick_ms >= TICK_MS) {
            game_update(&state);
            last_tick_ms = now;
        }

        /* Render */
        renderer_clear(&renderer);
        ui_draw_all(&renderer, &state);
        renderer_present(&renderer);
    }

    /* --- Cleanup --- */
    renderer_destroy(&renderer);
    return 0;
}