// dwarf.c
#include "dwarves.h"
#include "idle_core.h"

#define MAX_DWARVES 32

Dwarf dwarves[MAX_DWARVES];
int dwarf_count = 0;

void add_dwarf(ResourceType res) {
    if (dwarf_count >= MAX_DWARVES) return;

    dwarves[dwarf_count].assigned_to = res;
    dwarves[dwarf_count].production = 1;
    dwarf_count++;
}

void move_dwarf(int id, ResourceType new_resource) {
    if (id < 0 || id >= dwarf_count)
        return;
    dwarves[id].assigned_to = new_resource;
}

void dwarf_tick() {
    for (int i = 0; i < dwarf_count; i++) {
        switch (dwarves[i].assigned_to) {
            case RES_GOLD:
                gold += dwarves[i].production;
                break;
            case RES_STONE:
                stone += dwarves[i].production;
                break;
            case RES_IRON:
                iron += dwarves[i].production;
                break;
            default: break;
        }
    }
}
