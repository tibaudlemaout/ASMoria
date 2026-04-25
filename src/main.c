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

                /* ---- Mouse click: select a dwarf ---- */
                case SDL_MOUSEBUTTONDOWN:
                    if (ev.button.button == SDL_BUTTON_LEFT) {
                        int idx = ui_dwarf_at_pixel(&renderer, &state,
                                                    ev.button.x, ev.button.y);
                        /* toggle selection */
                        ui_selected_dwarf = (idx == ui_selected_dwarf) ? -1 : idx;
                    }
                    break;

                /* ---- Keyboard ---- */
                case SDL_KEYDOWN:
                    switch (ev.key.keysym.sym) {

                        case SDLK_ESCAPE:
                        case SDLK_q:
                            running = 0;
                            break;

                        /* Scroll log */
                        case SDLK_UP:   ui_log_scroll(+1); break;
                        case SDLK_DOWN: ui_log_scroll(-1); break;

                        /* Hire a dwarf */
                        case SDLK_h: {
                            int64_t idx = asm_hire_dwarf(&state);
                            if (idx < 0) {
                                /* push a dim event to log — not enough resources */
                                asm_event_push(&state, 0x09, EVT_FLAVOUR, 0xFF);
                            }
                            break;
                        }

                        /* Job assignment — only if a dwarf is selected */
                        case SDLK_m:
                        case SDLK_l:
                        case SDLK_f:
                        case SDLK_g:
                        case SDLK_s:
                        case SDLK_i: {
                            if (ui_selected_dwarf < 0) break;
                            uint8_t job = JOB_IDLE;
                            switch (ev.key.keysym.sym) {
                                case SDLK_m: job = JOB_MINER;    break;
                                case SDLK_l: job = JOB_LUMBERER; break;
                                case SDLK_f: job = JOB_FARMER;   break;
                                case SDLK_g: job = JOB_GUARD;    break;
                                case SDLK_s: job = JOB_SCHOLAR;  break;
                                case SDLK_i: job = JOB_IDLE;     break;
                                default: break;
                            }
                            asm_assign_job(&state,
                                           (uint8_t)ui_selected_dwarf, job);
                            break;
                        }

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