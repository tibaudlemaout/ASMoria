// dwarf.h
#ifndef DWARVES_H
#define DWARVES_H

#include "resources.h"
#include <stdint.h>

typedef struct {
    ResourceType assigned_to;
    uint32_t production;
} Dwarf;

extern Dwarf dwarves[];
extern int dwarf_count;

void add_dwarf(ResourceType job);
void move_dwarf(int id, ResourceType new_resource);
void dwarf_tick(void);

#endif
