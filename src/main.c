#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>

#include "../include/asmoria.h"
#include "render/renderer.h"
#include "ui/ui.h"
#include "ui/ui_upgrades.h"
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

                case SDL_MOUSEBUTTONDOWN:
                    if (ev.button.button == SDL_BUTTON_LEFT
                        && !ui_show_upgrades) {
                        int idx = ui_dwarf_at_pixel(&renderer, &state,
                                                    ev.button.x, ev.button.y);
                        ui_selected_dwarf = (idx == ui_selected_dwarf) ? -1 : idx;
                    }
                    break;

                case SDL_KEYDOWN:
                    switch (ev.key.keysym.sym) {

                        case SDLK_ESCAPE:
                        case SDLK_q:
                            if (ui_show_upgrades)
                                ui_show_upgrades = 0;
                            else
                                running = 0;
                            break;

                        /* Toggle upgrade panel */
                        case SDLK_u:
                            ui_show_upgrades = !ui_show_upgrades;
                            break;

                        /* Scroll log (main view) or navigate upgrades */
                        case SDLK_UP:
                            if (ui_show_upgrades) ui_upgr_move(-1);
                            else                  ui_log_scroll(+1);
                            break;
                        case SDLK_DOWN:
                            if (ui_show_upgrades) ui_upgr_move(+1);
                            else                  ui_log_scroll(-1);
                            break;

                        /* Buy upgrade */
                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:
                            if (ui_show_upgrades) {
                                int id = ui_upgr_selected();
                                if (id >= 0)
                                    asm_buy_upgrade(&state, (uint8_t)id);
                            }
                            break;

                        /* Hire (main view only) */
                        case SDLK_h:
                            if (!ui_show_upgrades) {
                                int64_t idx = asm_hire_dwarf(&state);
                                (void)idx;
                            }
                            break;

                        /* Job assignment */
                        case SDLK_m:
                        case SDLK_l:
                        case SDLK_f:
                        case SDLK_g:
                        case SDLK_s:
                        case SDLK_i: {
                            if (ui_show_upgrades || ui_selected_dwarf < 0)
                                break;
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
                    if (!ui_show_upgrades)
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