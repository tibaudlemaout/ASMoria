; =========================================================
; offsets.inc - GameState field offsets for NASM modules
; Verified against GCC offsetof() — sizeof(GameState) = 5936
; =========================================================

; --- Resources struct offsets (sizeof = 0x58 = 88) ---
%define RES_GOLD            0x00
%define RES_STONE           0x08
%define RES_WOOD            0x10
%define RES_FOOD            0x18
%define RES_MANA            0x20
%define RES_IRON_ORE        0x28    ; depth 2+
%define RES_GEMS            0x30    ; depth 3+
%define RES_RELICS          0x38    ; depth 4+
%define RES_CRYSTALS        0x40    ; depth 5+
%define RES_IRON_BARS       0x48    ; crafted from iron_ore
%define RES_ALE             0x50    ; crafted from food+wood
%define SIZEOF_RESOURCES    0x58

; --- Dwarf offsets (sizeof = 0x48 = 72) ---
%define DWARF_ALIVE         0x00
%define DWARF_JOB           0x01
%define DWARF_MORALE        0x02
%define DWARF_FATIGUE       0x03
%define DWARF_PREV_JOB      0x04
%define DWARF_JOB_LEVEL     0x05    ; uint8_t[7]
%define DWARF_NAME_IDX      0x0C    ; uint8_t — index into name table
%define DWARF_JOB_XP        0x10    ; int64_t[7] — index by job id * 8
%define SIZEOF_DWARF        0x48
%define MAX_DWARVES         64
%define JOB_COUNT           7
%define MAX_JOB_LEVEL       5

%define JOB_IDLE            0
%define JOB_MINER           1
%define JOB_LUMBERER        2
%define JOB_FARMER          3
%define JOB_GUARD           4
%define JOB_SCHOLAR         5
%define JOB_CRAFTSDWARF     6

%define FATIGUE_MAX         100
%define FATIGUE_WARN        85
%define MORALE_MAX          100
%define MORALE_IDLE_RATE    5

; --- GameState offsets (verified) ---
%define GS_RESOURCES        0x0000
%define GS_DWARVES          0x0058
%define GS_UPGR_TIER1       0x1258
%define GS_UPGR_TIER2       0x1260
%define GS_RNG_SEED         0x1268
%define GS_TICK             0x1270
%define GS_DEPTH            0x1278
%define GS_DEPTH_XP         0x127C
%define GS_FLAGS            0x1280
%define GS_PENDING          0x1284
%define GS_EVENTLOG         0x1290

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
%define XP_RATE_CRAFTSDWARF 2

; --- Upgrade IDs ---
%define UPGR_PICK_QUALITY   0
%define UPGR_SAW_QUALITY    1
%define UPGR_IRRIGATION     2
%define UPGR_BARRACKS       3
%define UPGR_RECRUITERS     4
%define UPGR_WATCH_TOWER    5
%define UPGR_RUNE_HALLS     6
%define UPGR_MANA_WELL      7
%define UPGR_VAULT          8
%define UPGR_WAREHOUSE      9
%define UPGR_GRANARY        10
%define UPGR_WORKSHOP       11
%define UPGR_COUNT          12

%define UPGR_MAX_TOOLS      3
%define UPGR_MAX_WORKFORCE  3
%define UPGR_MAX_WATCHTOWER 3
%define UPGR_MAX_RUNEHALLS  5
%define UPGR_MAX_MANAWELL   3
%define UPGR_MAX_STORAGE    3
%define UPGR_MAX_WORKSHOP   1

%define UPGR_COST_GOLD_TOOLS    150
%define UPGR_COST_STONE_TOOLS    75
%define UPGR_COST_WOOD_TOOLS     30
%define UPGR_COST_GOLD_WORK     200
%define UPGR_COST_STONE_WORK    100
%define UPGR_COST_GOLD_WATCH    200
%define UPGR_COST_STONE_WATCH   150
%define UPGR_COST_GOLD_RUNE     150
%define UPGR_COST_STONE_RUNE    100
%define UPGR_COST_MANA_RUNE      10
%define UPGR_COST_GOLD_MANA     200
%define UPGR_COST_STONE_MANA     50

; Storage buildings
%define UPGR_COST_GOLD_VAULT      200
%define UPGR_COST_STONE_VAULT     100
%define UPGR_COST_GOLD_WAREHOUSE  150
%define UPGR_COST_STONE_WAREHOUSE 150
%define UPGR_COST_WOOD_WAREHOUSE   50
%define UPGR_COST_GOLD_GRANARY    150
%define UPGR_COST_STONE_GRANARY   100
%define UPGR_COST_FOOD_GRANARY     50

; Workshop
%define UPGR_COST_GOLD_WORKSHOP   300
%define UPGR_COST_STONE_WORKSHOP  200

%define DWARF_CAP_BASE      16
%define DWARF_CAP_PER_LEVEL  8
%define HIRE_GOLD_BASE      50
%define HIRE_FOOD_BASE      20
%define HIRE_GOLD_DISCOUNT  10
%define HIRE_FOOD_DISCOUNT   5

; --- Storage caps ---
%define CAP_GOLD_BASE       500
%define CAP_STONE_BASE      500
%define CAP_WOOD_BASE       200
%define CAP_FOOD_BASE       200
%define CAP_MANA_BASE       100

%define CAP_GOLD_PER_LEVEL  500
%define CAP_STONE_PER_LEVEL 500
%define CAP_WOOD_PER_LEVEL  300
%define CAP_FOOD_PER_LEVEL  400

; --- Crafting rates ---
%define CRAFT_IRON_BAR_TICKS   5
%define CRAFT_IRON_ORE_COST    2
%define CRAFT_ALE_TICKS        3

; --- Depth system ---
%define DEPTH_MAX           5
%define DEPTH2_COST_STONE   500
%define DEPTH2_COST_GOLD    300
%define DEPTH3_COST_STONE  1500
%define DEPTH3_COST_GOLD   1000
%define DEPTH4_COST_STONE  3000
%define DEPTH4_COST_GOLD   2500
%define DEPTH5_COST_STONE  6000
%define DEPTH5_COST_GOLD   5000

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

%define RUNE_COST_ENDURANCE   150
%define RUNE_COST_PLENTY      200
%define RUNE_COST_SWIFTNESS   175
%define RUNE_COST_WARDING     250
%define RUNE_COST_KINSHIP     200
%define RUNE_COST_DEEP        400

; --- Raid / Breach system ---
%define GS_RAID             0x1698

%define RAID_ACTIVE         0x00
%define RAID_THREAT         0x01
%define RAID_GUARD_COUNT    0x02
%define RAID_ENEMY_HP       0x04
%define RAID_ENEMY_HP_MAX   0x08
%define RAID_ENEMY_ATK      0x0C
%define RAID_GUARD_IDX      0x10
%define RAID_GUARD_HP       0x18
%define RAID_GUARD_HP_MAX   0x38
%define RAID_NEXT_TICK      0x58
%define RAID_COMBAT_START   0x60
%define RAID_LAST_COMBAT    0x68
%define RAID_REWARD_GOLD    0x70
%define RAID_COMPLETED      0x74

%define RAID_NONE           0
%define RAID_WARNING        1
%define RAID_COMBAT         2
%define RAID_RESULT         3

%define RAID_MAX_GUARDS     8
%define RAID_WARN_TICKS     60
%define RAID_FIRST_TICK     300
%define RAID_INTERVAL       500
%define RAID_COMBAT_INTERVAL 5
%define RAID_TIMER          300

; --- Prestige system ---
%define GS_PRESTIGE         0x1710
%define PRESTIGE_HONOR      0x00
%define PRESTIGE_TOTAL_HON  0x04
%define PRESTIGE_COUNT      0x08
%define PRESTIGE_PAD        0x0C
%define PRESTIGE_NODES      0x10
%define PRESTIGE_RESOURCES  0x18

%define PNODE_YIELD_1       0
%define PNODE_YIELD_2       1
%define PNODE_YIELD_3       2
%define PNODE_START_GOLD_1  3
%define PNODE_START_GOLD_2  4
%define PNODE_START_STONE   5
%define PNODE_FOOD_TICK     6
%define PNODE_WOOD_TICK     7
%define PNODE_START_DWF_2   8
%define PNODE_START_DWF_3   9
%define PNODE_START_DWF_4   10
%define PNODE_HIRE_10       11
%define PNODE_HIRE_25       12
%define PNODE_START_PICK    13
%define PNODE_START_SAW     14
%define PNODE_START_IRR     15
%define PNODE_START_BARR    16
%define PNODE_RUNE_10       17
%define PNODE_RUNE_25       18
%define PNODE_START_MANA    19
%define PNODE_START_RUNE_H  20
%define PNODE_MANA_TICK_1   21
%define PNODE_MANA_TICK_2   22
%define PNODE_START_SCHOLAR 23
%define PNODE_COUNT         24

%define PRESTIGE_MIN_TICKS      1000
%define PRESTIGE_MIN_RAIDS      1
%define PRESTIGE_MIN_RUNES      3
%define PRESTIGE_MIN_RESOURCES  10000

%define GS_SIZE             5936