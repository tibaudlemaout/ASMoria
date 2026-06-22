; =========================================================
; offsets.inc - GameState field offsets for NASM modules
; Verified against GCC offsetof() -- sizeof(GameState) = 6224
; SAVE_VERSION = 10
; =========================================================

; --- Resources struct offsets (sizeof = 0x88 = 136) ---
%define RES_GOLD            0x00
%define RES_STONE           0x08
%define RES_WOOD            0x10
%define RES_FOOD            0x18
%define RES_MANA            0x20
%define RES_IRON_ORE        0x28
%define RES_GEMS            0x30
%define RES_RELICS          0x38
%define RES_CRYSTALS        0x40
%define RES_IRON_BARS       0x48
%define RES_ALE             0x50
%define RES_WEAPONS         0x58
%define RES_ARMOUR          0x60
%define RES_TOOLS           0x68
%define RES_WALLS           0x70
%define RES_SPIKE_TRAPS     0x78
%define RES_SLOW_TRAPS      0x80
%define SIZEOF_RESOURCES    0x88

; --- Dwarf offsets (sizeof = 0x48 = 72) ---
%define DWARF_ALIVE         0x00
%define DWARF_JOB           0x01
%define DWARF_MORALE        0x02
%define DWARF_FATIGUE       0x03
%define DWARF_PREV_JOB      0x04
%define DWARF_JOB_LEVEL     0x05
%define DWARF_NAME_IDX      0x0C
%define DWARF_EQUIPMENT     0x0D
%define DWARF_JOB_XP        0x10
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

%define EQUIP_NONE          0
%define EQUIP_WEAPON        1
%define EQUIP_ARMOUR        2
%define EQUIP_TOOL          3

%define WEAPON_ATK_BONUS    10
%define ARMOUR_HP_BONUS     20
%define TOOL_YIELD_BONUS    15

%define FATIGUE_MAX         100
%define FATIGUE_WARN        85
%define MORALE_MAX          100
%define MORALE_IDLE_RATE    5

; --- GameState offsets ---
; craft[RECIPE_COUNT] grew from 6 to 9 slots (+12 bytes).
; The extra slots also change the alignment of RngState (uint64_t seed),
; which needs GCC to insert 4 bytes of padding after tavern_level.
; Net shift: +12 for TAVERN_BUFFS/TAVERN_LEVEL, +16 for everything else.
%define GS_RESOURCES        0x0000
%define GS_DWARVES          0x0088
%define GS_UPGR_TIER1       0x1288
%define GS_UPGR_TIER2       0x1290
%define GS_CRAFT            0x1298
%define GS_TAVERN_BUFFS     0x12bc
%define GS_TAVERN_LEVEL     0x12c8
%define GS_RNG_SEED         0x12d0
%define GS_TICK             0x12d8
%define GS_DEPTH            0x12e0
%define GS_DEPTH_XP         0x12e4
%define GS_FLAGS            0x12e8
%define GS_PENDING          0x12ec
%define GS_EVENTLOG         0x12f8
%define GS_RAID             0x1700
%define GS_PRESTIGE         0x1830
%define GS_SIZE             6224

; --- Raid struct offsets (from GS_RAID base) ---
%define RAID_ACTIVE             0x00
%define RAID_THREAT             0x01
%define RAID_ENEMIES_REM        0x02
%define RAID_ENEMIES_REMAINING  0x02
%define RAID_SETTLEMENT_BREACH  0x03
%define RAID_COMPLETED          0x04
%define RAID_NEXT_TICK          0x08
%define RAID_LAST_MOVE_TICK     0x10
%define RAID_GRID               0x18
%define RAID_ENEMIES            0x54
%define RAID_GUARDS             0xb4
%define RAID_GUARD_COUNT        0xf4
%define RAID_WALLS              0xf6
%define RAID_WALL_COUNT         0x11e
%define RAID_REWARD_GOLD        0x120
%define RAID_REWARD_RELICS      0x124
%define RAID_CURSOR_COL         0x128
%define RAID_CURSOR_ROW         0x129
%define RAID_PLACE_MODE         0x12a

; --- Enemy struct offsets ---
%define ENEMY_TYPE          0x00
%define ENEMY_COL           0x01
%define ENEMY_ROW           0x02
%define ENEMY_SLOW_TIMER    0x03
%define ENEMY_HP            0x04
%define ENEMY_HP_MAX        0x06
%define ENEMY_ATK           0x08
%define ENEMY_MOVE_TIMER    0x09
%define ENEMY_SPAWN_DELAY   0x0a
%define SIZEOF_ENEMY        0x0c

; --- RaidGuard struct offsets ---
%define RGUARD_DWARF_IDX    0x00
%define RGUARD_COL          0x01
%define RGUARD_ROW          0x02
%define RGUARD_ACTIVE       0x03
%define RGUARD_HP           0x04
%define RGUARD_HP_MAX       0x06
%define SIZEOF_RAIDGUARD    0x08

; --- RaidWall struct offsets ---
%define RWALL_COL           0x00
%define RWALL_ROW           0x01
%define RWALL_HP            0x02
%define SIZEOF_RAIDWALL     0x04

; --- Grid / Cell constants ---
%define RAID_COLS           12
%define RAID_ROWS           5
%define RAID_COL_SPAWN      0
%define RAID_COL_SETTLE     11

%define CELL_EMPTY          0
%define CELL_GUARD          1
%define CELL_WALL           2
%define CELL_SPIKE_TRAP     3
%define CELL_SLOW_TRAP      4
%define CELL_SETTLEMENT     5

; --- Enemy type constants ---
%define ENEMY_NONE              0
%define ENEMY_GOBLIN_SCOUT      1
%define ENEMY_GOBLIN_RAIDER     2
%define ENEMY_STONE_TROLL       3
%define ENEMY_WAR_TROLL         4
%define ENEMY_DEMON             5

%define RAID_MAX_ENEMIES    8
%define RAID_MAX_GUARDS     8
%define RAID_MAX_WALLS      10

%define RAID_NONE           0
%define RAID_WARNING        1
%define RAID_COMBAT         2
%define RAID_RESULT         3

%define RAID_WARN_TICKS     120
%define RAID_FIRST_TICK     300
%define RAID_INTERVAL       500
%define RAID_MOVE_INTERVAL  8
%define RAID_SLOW_TURNS     4

; --- Pending event ---
%define PENDING_CODE        0x00
%define PENDING_SEVERITY    0x01
%define PENDING_DWARF       0x02

; --- Event log ---
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

; --- XP ---
%define XP_LVL2             500
%define XP_LVL3             1200
%define XP_LVL4             2500
%define XP_LVL5             4500
%define XP_RATE_IDLE        2
%define XP_RATE_MINER       3
%define XP_RATE_LUMBERER    3
%define XP_RATE_FARMER      2
%define XP_RATE_GUARD       2
%define XP_RATE_SCHOLAR     2
%define XP_RATE_CRAFTSDWARF 2

; --- Upgrade IDs ---
%define UPGR_PICK_QUALITY    0
%define UPGR_SAW_QUALITY     1
%define UPGR_IRRIGATION      2
%define UPGR_BARRACKS        3
%define UPGR_RECRUITERS      4
%define UPGR_WATCH_TOWER     5
%define UPGR_RUNE_HALLS      6
%define UPGR_MANA_WELL       7
%define UPGR_VAULT           8
%define UPGR_WAREHOUSE       9
%define UPGR_GRANARY         10
%define UPGR_WORKSHOP        11
%define UPGR_TAVERN          12
%define UPGR_ORE_STORAGE     13
%define UPGR_FORGE           14
%define UPGR_BREWERY         15
%define UPGR_DEEP_BARRACKS   16
%define UPGR_RELIC_VAULT     17
%define UPGR_CRYSTAL_CONDUIT 18
%define UPGR_OUTPOSTS        19
%define UPGR_COUNT           20

%define UPGR_MAX_TOOLS           5
%define UPGR_MAX_WORKFORCE       3
%define UPGR_MAX_WATCHTOWER      5
%define UPGR_MAX_RUNEHALLS       5
%define UPGR_MAX_MANAWELL        5
%define UPGR_MAX_STORAGE         3
%define UPGR_MAX_WORKSHOP        1
%define UPGR_MAX_TAVERN          3
%define UPGR_MAX_ORE_STORAGE     3
%define UPGR_MAX_FORGE           3
%define UPGR_MAX_BREWERY         3
%define UPGR_MAX_DEEP_BARRACKS   3
%define UPGR_MAX_RELIC_VAULT     3
%define UPGR_MAX_CRYSTAL_CONDUIT 3
%define UPGR_MAX_OUTPOSTS        5

%define UPGR_COST_GOLD_TOOLS      150
%define UPGR_COST_STONE_TOOLS      75
%define UPGR_COST_WOOD_TOOLS       30
%define UPGR_COST_GOLD_WORK       200
%define UPGR_COST_STONE_WORK      100
%define UPGR_COST_GOLD_WATCH      200
%define UPGR_COST_STONE_WATCH     150
%define UPGR_COST_GOLD_RUNE       150
%define UPGR_COST_STONE_RUNE      100
%define UPGR_COST_MANA_RUNE        10
%define UPGR_COST_GOLD_MANA       200
%define UPGR_COST_STONE_MANA       50
%define UPGR_COST_GOLD_VAULT      200
%define UPGR_COST_STONE_VAULT     100
%define UPGR_COST_GOLD_WAREHOUSE  150
%define UPGR_COST_STONE_WAREHOUSE 150
%define UPGR_COST_WOOD_WAREHOUSE   50
%define UPGR_COST_GOLD_GRANARY    150
%define UPGR_COST_STONE_GRANARY   100
%define UPGR_COST_FOOD_GRANARY     50
%define UPGR_COST_GOLD_WORKSHOP   300
%define UPGR_COST_STONE_WORKSHOP  200

%define DWARF_CAP_BASE       16
%define DWARF_CAP_PER_LEVEL   8
%define HIRE_GOLD_BASE        50
%define HIRE_FOOD_BASE        20
%define HIRE_GOLD_DISCOUNT    10
%define HIRE_FOOD_DISCOUNT     5

; --- Storage caps ---
%define CAP_GOLD_BASE        500
%define CAP_STONE_BASE       500
%define CAP_WOOD_BASE        200
%define CAP_FOOD_BASE        200
%define CAP_MANA_BASE        100
%define CAP_GOLD_PER_LEVEL   500
%define CAP_STONE_PER_LEVEL  500
%define CAP_WOOD_PER_LEVEL   300
%define CAP_FOOD_PER_LEVEL   400

; --- TavernBuff ---
%define TBUFF_ACTIVE        0x00
%define TBUFF_ID            0x01
%define TBUFF_TIMER         0x02
%define SIZEOF_TAVAVERBUFF  0x04
%define MAX_TAVERN_BUFFS    3
%define TAVERN_MAX_LEVEL    3

%define BUFF_NONE              0
%define BUFF_FEAST             1
%define BUFF_DRINKING_CONTEST  2
%define BUFF_TOAST             3
%define BUFF_LONG_REST         4
%define BUFF_MINERS_COURAGE    5
%define BUFF_LUMBERJACKS_SONG  6
%define BUFF_HARVEST_FESTIVAL  7
%define BUFF_WARRIORS_DRAFT    8
%define BUFF_SCHOLARS_CLARITY  9
%define BUFF_CRAFTERS_FOCUS    10
%define BUFF_COUNT             11

; --- CraftSlot ---
%define CRAFT_ASSIGNED      0x00
%define CRAFT_ACTIVE        0x01
%define CRAFT_TIMER         0x02
%define SIZEOF_CRAFTSLOT    0x04
%define RECIPE_COUNT        9

%define RECIPE_IRON_BARS_I  0
%define RECIPE_ALE_I        1
%define RECIPE_IRON_BARS_II 2
%define RECIPE_WEAPONS_I    3
%define RECIPE_ARMOUR_I     4
%define RECIPE_TOOLS_I      5
%define RECIPE_WALL_I       6
%define RECIPE_SPIKE_I      7
%define RECIPE_SLOW_I       8

; --- Resource indices (RES_x / 8) ---
%define RID_GOLD            0
%define RID_STONE           1
%define RID_WOOD            2
%define RID_FOOD            3
%define RID_MANA            4
%define RID_IRON_ORE        5
%define RID_GEMS            6
%define RID_RELICS          7
%define RID_CRYSTALS        8
%define RID_IRON_BARS       9
%define RID_ALE             10
%define RID_WEAPONS         11
%define RID_ARMOUR          12
%define RID_TOOLS           13
%define RID_WALLS           14
%define RID_SPIKE_TRAPS     15
%define RID_SLOW_TRAPS      16

; --- Depth system ---
%define DEPTH_MAX           5
%define DEPTH2_COST_STONE   500
%define DEPTH2_COST_GOLD    300
%define DEPTH3_COST_STONE   1500
%define DEPTH3_COST_GOLD    1000
%define DEPTH4_COST_STONE   3000
%define DEPTH4_COST_GOLD    2500
%define DEPTH5_COST_STONE   6000
%define DEPTH5_COST_GOLD    5000

; --- Rune system ---
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

; --- Prestige ---
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