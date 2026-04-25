#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>

#include "../include/asmoria.h"
#include "render/renderer.h"
#include "ui/ui.h"
#include "game/game.h"

int main(void) {
    Renderer  renderer;
    GameState state;

    if (renderer_init(&renderer, "ASMoria") != 0)
        return 1;

    game_init(&state);

    int      running      = 1;
    uint64_t last_tick_ms = SDL_GetTicks64();

    while (running) {
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
                        case SDLK_UP:
                            ui_log_scroll(+1);
                            break;
                        case SDLK_DOWN:
                            ui_log_scroll(-1);
                            break;
                        default:
                            break;
                    }
                    break;

                case SDL_MOUSEWHEEL:
                    ui_log_scroll(ev.wheel.y > 0 ? +3 : -3);
                    break;

                default:
                    break;
            }
        }

        uint64_t now = SDL_GetTicks64();
        if (now - last_tick_ms >= TICK_MS) {
            game_update(&state);
            last_tick_ms = now;
        }

        renderer_clear(&renderer);
        ui_draw_all(&renderer, &state);
        renderer_present(&renderer);
    }

    renderer_destroy(&renderer);
    return 0;
}