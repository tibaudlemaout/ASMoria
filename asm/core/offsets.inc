; =========================================================
; offsets.inc - GameState field offsets for NASM modules
;
; KEEP IN SYNC with include/asmoria.h
; Offsets verified against GCC output via offsetof()
; =========================================================

; --- Resources offsets (base = &state->resources) ---
%define RES_GOLD            0x00
%define RES_STONE           0x08
%define RES_WOOD            0x10
%define RES_FOOD            0x18
%define RES_MANA            0x20
%define SIZEOF_RESOURCES    0x28

; --- Dwarf offsets (base = &state->dwarves[i]) ---
%define DWARF_ALIVE         0x00
%define DWARF_JOB           0x01
%define DWARF_MORALE        0x02
%define DWARF_FATIGUE       0x03
%define DWARF_XP            0x08
%define SIZEOF_DWARF        0x10
%define MAX_DWARVES         64

; --- Job constants ---
%define JOB_IDLE            0
%define JOB_MINER           1
%define JOB_LUMBERER        2
%define JOB_FARMER          3
%define JOB_GUARD           4
%define JOB_SCHOLAR         5

; --- GameState field offsets (base = state ptr) ---
; Verified: sizeof(GameState)=2136
%define GS_RESOURCES        0x0000      ; offset   0
%define GS_DWARVES          0x0028      ; offset  40
%define GS_UPGRADES         0x0428      ; offset 1064
%define GS_RNG_SEED         0x0438      ; offset 1080
%define GS_TICK             0x0440      ; offset 1088
%define GS_DEPTH            0x0448      ; offset 1096
%define GS_FLAGS            0x044C      ; offset 1100
%define GS_EVENTLOG         0x0450      ; offset 1104

; --- EventLog offsets (base = &state->event_log) ---
; sizeof(EventLog)=1032, sizeof(EventRecord)=16
%define EVTLOG_ENTRIES      0x000       ; offset    0
%define EVTLOG_HEAD         0x400       ; offset 1024
%define EVTLOG_COUNT        0x401       ; offset 1025
%define EVENT_LOG_SIZE      64
%define SIZEOF_EVENTRECORD  0x10

; --- EventRecord offsets (base = &entries[i]) ---
%define EVT_TICK            0x00
%define EVT_CODE            0x08
%define EVT_SEVERITY        0x09
%define EVT_DWARF_IDX       0x0A

; --- Event severity constants ---
%define EVT_FLAVOUR         0
%define EVT_POSITIVE        1
%define EVT_NEGATIVE        2
%define EVT_MILESTONE       3

; --- Event firing interval ---
%define EVENT_INTERVAL      30