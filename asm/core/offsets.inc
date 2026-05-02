; =========================================================
; offsets.inc - GameState field offsets for NASM modules
; Verified against GCC with new per-job XP Dwarf layout
; =========================================================

%define RES_GOLD            0x00
%define RES_STONE           0x08
%define RES_WOOD            0x10
%define RES_FOOD            0x18
%define RES_MANA            0x20
%define SIZEOF_RESOURCES    0x28

; --- Dwarf offsets (sizeof = 64 = 0x40) ---
%define DWARF_ALIVE         0x00
%define DWARF_JOB           0x01
%define DWARF_MORALE        0x02
%define DWARF_FATIGUE       0x03
%define DWARF_PREV_JOB      0x04
%define DWARF_JOB_LEVEL     0x05
%define DWARF_NAME_IDX      0x0B    ; uint8_t — index into name table
%define DWARF_JOB_XP        0x10    ; int64_t[6] — index by job id * 8
%define SIZEOF_DWARF        0x40
%define MAX_DWARVES         64
%define JOB_COUNT           6
%define MAX_JOB_LEVEL       5

%define JOB_IDLE            0
%define JOB_MINER           1
%define JOB_LUMBERER        2
%define JOB_FARMER          3
%define JOB_GUARD           4
%define JOB_SCHOLAR         5

%define FATIGUE_MAX         100
%define FATIGUE_WARN        85
%define MORALE_MAX          100
%define MORALE_IDLE_RATE    5

; --- GameState offsets (verified) ---
%define GS_RESOURCES        0x0000
%define GS_DWARVES          0x0028
%define GS_UPGR_TIER1       0x1028
%define GS_RNG_SEED         0x1038
%define GS_TICK             0x1040
%define GS_DEPTH            0x1048
%define GS_FLAGS            0x104C
%define GS_PENDING          0x1050
%define GS_EVENTLOG         0x1058

%define PENDING_CODE        0x00
%define PENDING_SEVERITY    0x01
%define PENDING_DWARF       0x02

%define EVTLOG_ENTRIES      0x000
%define EVTLOG_HEAD         0x400
%define EVTLOG_COUNT        0x401
%define EVENT_LOG_SIZE      64
%define SIZEOF_EVENTRECORD  0x10

%define EVT_TICK            0x00
%define EVT_CODE            0x08
%define EVT_SEVERITY        0x09
%define EVT_DWARF_IDX       0x0A

%define EVT_FLAVOUR         0
%define EVT_POSITIVE        1
%define EVT_NEGATIVE        2
%define EVT_MILESTONE       3

%define EVENT_INTERVAL      30

; --- XP thresholds ---
%define XP_LVL2             500
%define XP_LVL3             1200
%define XP_LVL4             2500
%define XP_LVL5             4500

; --- XP gain per tick by job ---
%define XP_RATE_IDLE        2
%define XP_RATE_MINER       3
%define XP_RATE_LUMBERER    3
%define XP_RATE_FARMER      2
%define XP_RATE_GUARD       2
%define XP_RATE_SCHOLAR     2

; --- Upgrade IDs ---
%define UPGR_PICK_QUALITY   0
%define UPGR_SAW_QUALITY    1
%define UPGR_IRRIGATION     2
%define UPGR_BARRACKS       3
%define UPGR_RECRUITERS     4
%define UPGR_WATCH_TOWER    5
%define UPGR_RUNE_HALLS     6
%define UPGR_MANA_WELL      7
%define UPGR_COUNT          8

%define UPGR_MAX_TOOLS      3
%define UPGR_MAX_WORKFORCE  3
%define UPGR_MAX_WATCHTOWER 3
%define UPGR_MAX_RUNEHALLS  5
%define UPGR_MAX_MANAWELL   3

%define UPGR_COST_GOLD_TOOLS    100
%define UPGR_COST_STONE_TOOLS    50
%define UPGR_COST_GOLD_WORK     150
%define UPGR_COST_STONE_WORK     75
%define UPGR_COST_GOLD_WATCH    200
%define UPGR_COST_STONE_WATCH   150
%define UPGR_COST_GOLD_RUNE     150
%define UPGR_COST_STONE_RUNE    100
%define UPGR_COST_MANA_RUNE      10
%define UPGR_COST_GOLD_MANA     200
%define UPGR_COST_STONE_MANA     50

%define DWARF_CAP_BASE      16
%define DWARF_CAP_PER_LEVEL 16
%define HIRE_GOLD_BASE      50
%define HIRE_FOOD_BASE      20
%define HIRE_GOLD_DISCOUNT  10
%define HIRE_FOOD_DISCOUNT   5

; --- Research / Rune system (packed in tier2) ---
%define RUNE_ENDURANCE      0
%define RUNE_PLENTY         1
%define RUNE_SWIFTNESS      2
%define RUNE_WARDING        3
%define RUNE_KINSHIP        4
%define RUNE_DEEP           5
%define RUNE_COUNT          6

%define RUNE_MAX_SMALL      5
%define RUNE_MAX_LARGE      3

%define RUNE_COST_ENDURANCE   50
%define RUNE_COST_PLENTY      75
%define RUNE_COST_SWIFTNESS   60
%define RUNE_COST_WARDING    100
%define RUNE_COST_KINSHIP     80
%define RUNE_COST_DEEP       150

%define GS_UPGR_TIER2       0x1030

; save.asm GS_SIZE must match sizeof(GameState)
%define GS_SIZE             5216