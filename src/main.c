#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>

#include "../include/asmoria.h"
#include "render/renderer.h"
#include "ui/ui.h"
#include "ui/ui_upgrades.h"
#include "ui/ui_research.h"
#include "ui/ui_breach.h"
#include "ui/ui_prestige.h"
#include "ui/ui_craft.h"
#include "ui/ui_tavern.h"
#include "game/game.h"
#include "game/save.h"

int main(void) {
    Renderer  renderer;
    GameState state;

    if (renderer_init(&renderer, "ASMoria") != 0)
        return 1;

    /* Load save or start fresh */
    game_load_or_init(&state);

    int      running      = 1;
    uint64_t last_tick_ms = SDL_GetTicks64();

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {

                case SDL_QUIT:
                    /* Autosave on quit */
                    save_game(&state);
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
                            else if (ui_show_research)
                                ui_show_research = 0;
                            else if (ui_show_breach)
                                ui_show_breach = 0;
                            else if (ui_show_prestige)
                                ui_show_prestige = 0;
                            else if (ui_show_craft)
                                ui_show_craft = 0;
                            else if (ui_show_tavern)
                                ui_show_tavern = 0;
                            else {
                                save_game(&state);
                                running = 0;
                            }
                            break;

                        case SDLK_r:
                            if (ui_show_breach)
                                asm_breach_retreat(&state);
                            else if (!ui_show_upgrades)
                                ui_show_research = !ui_show_research;
                            break;

                        case SDLK_p:
                            if (!ui_show_upgrades && !ui_show_research && !ui_show_breach)
                                ui_show_prestige = !ui_show_prestige;
                            break;

                        case SDLK_w:
                            if (!ui_show_upgrades && !ui_show_research
                                && !ui_show_breach && !ui_show_prestige)
                                ui_show_craft = !ui_show_craft;
                            break;

                        case SDLK_t:
                            if (!ui_show_upgrades && !ui_show_research
                                && !ui_show_breach && !ui_show_prestige
                                && !ui_show_craft)
                                ui_show_tavern = !ui_show_tavern;
                            break;

                        case SDLK_b:
                            if (!ui_show_upgrades && !ui_show_research) {
                                ui_show_breach = !ui_show_breach;
                                /* result acknowledged */
                                /* Clear RESULT state when player dismisses */
                                if (!ui_show_breach && state.raid.active == RAID_RESULT)
                                    state.raid.active = RAID_NONE;
                            }
                            break;

                        /* Manual save */
                        case SDLK_F5: {
                            SaveResult r = save_game(&state);
                            if (r == SAVE_OK)
                                asm_event_push(&state, 0x4B, EVT_MILESTONE, 0xFF);
                            else
                                fprintf(stderr, "[save] %s\n",
                                        save_result_str(r));
                            break;
                        }

                        /* Manual load */
                        case SDLK_F9: {
                            SaveResult r = load_game(&state);
                            if (r == SAVE_OK)
                                asm_event_push(&state, 0x4B, EVT_MILESTONE, 0xFF);
                            else if (r == SAVE_ERR_NOFILE)
                                asm_event_push(&state, 0x42, EVT_MILESTONE, 0xFF);
                            else {
                                fprintf(stderr, "[save] Load failed: %s\n",
                                        save_result_str(r));
                                asm_event_push(&state, 0x4A, EVT_NEGATIVE, 0xFF);
                            }
                            break;
                        }

                        case SDLK_u:
                            ui_show_upgrades = !ui_show_upgrades;
                            break;

                        case SDLK_UP:
                            if (ui_show_upgrades)       ui_upgr_move(-1);
                            else if (ui_show_research)  ui_research_move(-1);
                            else if (ui_show_breach)    ui_breach_move(0, -1);
                            else if (ui_show_prestige)  ui_prestige_move(-1);
                            else if (ui_show_craft)      ui_craft_move(-1);
                            else if (ui_show_tavern)     ui_tavern_move(-1);
                            else                        ui_dwarf_select(-1);
                            break;
                        case SDLK_DOWN:
                            if (ui_show_upgrades)       ui_upgr_move(+1);
                            else if (ui_show_research)  ui_research_move(+1);
                            else if (ui_show_breach)    ui_breach_move(0, +1);
                            else if (ui_show_prestige)  ui_prestige_move(+1);
                            else if (ui_show_craft)      ui_craft_move(+1);
                            else if (ui_show_tavern)     ui_tavern_move(+1);
                            else                        ui_dwarf_select(+1);
                            break;
                        case SDLK_LEFT:
                            if (ui_show_breach) ui_breach_move(-1, 0);
                            else if (!ui_show_upgrades) ui_dwarf_scroll(-1);
                            break;
                        case SDLK_RIGHT:
                            if (ui_show_breach) ui_breach_move(+1, 0);
                            else if (!ui_show_upgrades) ui_dwarf_scroll(+1);
                            break;
                        case SDLK_PAGEUP:
                            if (!ui_show_upgrades) ui_log_scroll(+3);
                            break;
                        case SDLK_PAGEDOWN:
                            if (!ui_show_upgrades) ui_log_scroll(-3);
                            break;

                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:
                            if (ui_show_upgrades) {
                                int id = ui_upgr_selected();
                                if (id >= 0)
                                    asm_buy_upgrade(&state, (uint8_t)id);
                            } else if (ui_show_research) {
                                int id = ui_research_selected();
                                if (id >= 0)
                                    asm_buy_rune(&state, (uint8_t)id);
                            } else if (ui_show_prestige) {
                                int id = ui_prestige_selected();
                                if (id >= 0 && id < PNODE_COUNT)
                                    asm_buy_pnode(&state, (uint8_t)id);
                                else if (id == PNODE_COUNT)
                                    asm_do_prestige(&state);
                            }
                            break;

                        case SDLK_h:
                            if (!ui_show_upgrades)
                                asm_hire_dwarf(&state);
                            break;

                        case SDLK_d:
                            if (!ui_show_upgrades && !ui_show_research
                                && !ui_show_breach && !ui_show_prestige)
                                asm_dig_deeper(&state);
                            break;

                        case SDLK_EQUALS:
                        case SDLK_PLUS:
                            if (ui_show_craft)
                                asm_craft_assign(&state, (uint8_t)ui_craft_selected(), 1);
                            break;

                        case SDLK_MINUS:
                            if (ui_show_craft)
                                asm_craft_assign(&state, (uint8_t)ui_craft_selected(), -1);
                            break;

                        case SDLK_1:
                            if (!ui_show_upgrades && !ui_show_research
                                && !ui_show_breach && !ui_show_prestige
                                && !ui_show_craft && ui_selected_dwarf >= 0)
                                asm_equip(&state, (uint64_t)ui_selected_dwarf, EQUIP_WEAPON);
                            break;

                        case SDLK_2:
                            if (!ui_show_upgrades && !ui_show_research
                                && !ui_show_breach && !ui_show_prestige
                                && !ui_show_craft && ui_selected_dwarf >= 0)
                                asm_equip(&state, (uint64_t)ui_selected_dwarf, EQUIP_ARMOUR);
                            break;

                        case SDLK_3:
                            if (!ui_show_upgrades && !ui_show_research
                                && !ui_show_breach && !ui_show_prestige
                                && !ui_show_craft && ui_selected_dwarf >= 0)
                                asm_equip(&state, (uint64_t)ui_selected_dwarf, EQUIP_TOOL);
                            break;

                        case SDLK_0:
                            if (!ui_show_upgrades && !ui_show_research
                                && !ui_show_breach && !ui_show_prestige
                                && !ui_show_craft && ui_selected_dwarf >= 0)
                                asm_unequip(&state, (uint64_t)ui_selected_dwarf);
                            break;

                        case SDLK_e:
                            if (!ui_show_upgrades && !ui_show_research
                                && ui_selected_dwarf >= 0) {
                                asm_feed_dwarf(&state, (uint8_t)ui_selected_dwarf);
                            }
                            break;

                        case SDLK_m:
                        case SDLK_l:
                        case SDLK_f:
                        case SDLK_g:
                        case SDLK_s:
                        case SDLK_c:
                        case SDLK_i: {
                            if (ui_show_upgrades || ui_show_research
                                || ui_show_breach || ui_show_prestige
                                || ui_selected_dwarf < 0)
                                break;
                            uint8_t job = JOB_IDLE;
                            switch (ev.key.keysym.sym) {
                                case SDLK_m: job = JOB_MINER;       break;
                                case SDLK_l: job = JOB_LUMBERER;    break;
                                case SDLK_f: job = JOB_FARMER;      break;
                                case SDLK_g: job = JOB_GUARD;       break;
                                case SDLK_s: job = JOB_SCHOLAR;     break;
                                case SDLK_c: job = JOB_CRAFTSDWARF; break;
                                case SDLK_i: job = JOB_IDLE;        break;
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

        /* Flashing red border during combat — toggles every 500ms */
        if (state.raid.active == RAID_COMBAT) {
            uint64_t flash = SDL_GetTicks64() / 500;
            if (flash & 1)
                renderer_draw_screen_border(&renderer, 0xFF2222FF, 3);
        }

        renderer_present(&renderer);
    }

    renderer_destroy(&renderer);
    return 0;
}
