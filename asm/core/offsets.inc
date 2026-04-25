; =========================================================
; offsets.inc - GameState field offsets for NASM modules
;
; KEEP IN SYNC with include/asmoria.h
; Any struct change there must be reflected here.
; =========================================================

; --- Resources offsets (base = &state->resources) ---
%define RES_GOLD        0x00
%define RES_STONE       0x08
%define RES_WOOD        0x10
%define RES_FOOD        0x18
%define RES_MANA        0x20
%define SIZEOF_RESOURCES 0x28

; --- Dwarf offsets (base = &state->dwarves[i]) ---
%define DWARF_ALIVE     0x00
%define DWARF_JOB       0x01
%define DWARF_MORALE    0x02
%define DWARF_FATIGUE   0x03
%define DWARF_XP        0x08
%define SIZEOF_DWARF    0x10
%define MAX_DWARVES     64

; --- Job constants ---
%define JOB_IDLE        0
%define JOB_MINER       1
%define JOB_LUMBERER    2
%define JOB_FARMER      3
%define JOB_GUARD       4
%define JOB_SCHOLAR     5

; --- GameState field offsets (base = state ptr) ---
%define GS_RESOURCES    0x0000
%define GS_DWARVES      0x0028
%define GS_UPGRADES     0x0828
%define GS_RNG_SEED     0x0838
%define GS_TICK         0x0840
%define GS_DEPTH        0x0848
%define GS_FLAGS        0x084C