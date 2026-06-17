#ifndef ASMORIA_H
#define ASMORIA_H

#include <stdint.h>

typedef struct {
    int64_t gold;
    int64_t stone;
    int64_t wood;
    int64_t food;
    int64_t mana;
    int64_t iron_ore;   /* depth 2+ */
    int64_t gems;       /* depth 3+ */
    int64_t relics;     /* depth 4+ */
    int64_t crystals;   /* depth 5+ */
    int64_t iron_bars;  /* crafted from iron_ore */
    int64_t ale;        /* crafted from food+wood */
} Resources;

#define MAX_DWARVES     64
#define JOB_COUNT       7       /* idle=0 miner=1 lumberer=2 farmer=3 guard=4 scholar=5 craftsdwarf=6 */
#define MAX_JOB_LEVEL   5

typedef struct {
    uint8_t  alive;             /* +0x00 */
    uint8_t  job;               /* +0x01 */
    uint8_t  morale;            /* +0x02 */
    uint8_t  fatigue;           /* +0x03 */
    uint8_t  prev_job;          /* +0x04 */
    uint8_t  job_level[7];      /* +0x05 : level per job (indexed by JOB_*, incl. Craftsdwarf) */
    uint8_t  name_idx;          /* +0x0C : index into dwarf name table */
    uint8_t  equipment;         /* +0x0D : EQUIP_* constant */
    uint8_t  _pad2[2];          /* +0x0E : padding to align job_xp on 8 bytes */
    int64_t  job_xp[7];         /* +0x10 : xp per job (incl. Craftsdwarf) */
} Dwarf;                        /* size: 64 bytes */

#define JOB_IDLE     0
#define JOB_MINER    1
#define JOB_LUMBERER 2
#define JOB_FARMER   3
#define JOB_GUARD    4
#define JOB_SCHOLAR  5
#define JOB_CRAFTSDWARF 6

/* XP thresholds for each level (index = level, value = xp needed) */
#define XP_LVL1     0
#define XP_LVL2     500
#define XP_LVL3     1500
#define XP_LVL4     3500
#define XP_LVL5     7500

/* Equipment slots */
#define EQUIP_NONE      0
#define EQUIP_WEAPON    1
#define EQUIP_ARMOUR    2
#define EQUIP_TOOL      3

/* Recipe IDs */
#define RECIPE_IRON_BARS_I   0
#define RECIPE_ALE_I         1
#define RECIPE_IRON_BARS_II  2
#define RECIPE_WEAPONS_I     3
#define RECIPE_ARMOUR_I      4
#define RECIPE_TOOLS_I       5
#define RECIPE_COUNT         6

/* Craft slot — one per recipe */
typedef struct {
    uint8_t  assigned;   /* dwarves assigned */
    uint8_t  active;     /* 1 = timer running */
    uint16_t timer;      /* ticks remaining */
} CraftSlot;

/* Storage caps -- default values, increased by Vault/Warehouse/Granary */
#define CAP_GOLD_BASE       500
#define CAP_STONE_BASE      500
#define CAP_WOOD_BASE       200
#define CAP_FOOD_BASE       200
#define CAP_MANA_BASE       100

#define CAP_GOLD_PER_LEVEL  500
#define CAP_STONE_PER_LEVEL 500
#define CAP_WOOD_PER_LEVEL  300
#define CAP_FOOD_PER_LEVEL  400

#define UPGR_MAX_STORAGE    3

/* Storage buildings — cost scales linearly per level like tools */
#define UPGR_COST_GOLD_VAULT      200
#define UPGR_COST_STONE_VAULT     100
#define UPGR_COST_GOLD_WAREHOUSE  150
#define UPGR_COST_STONE_WAREHOUSE 150
#define UPGR_COST_WOOD_WAREHOUSE   50
#define UPGR_COST_GOLD_GRANARY    150
#define UPGR_COST_STONE_GRANARY   100
#define UPGR_COST_FOOD_GRANARY     50

/* Workshop (unlocks Craftsdwarf) */
#define UPGR_COST_GOLD_WORKSHOP   300
#define UPGR_COST_STONE_WORKSHOP  200
#define UPGR_MAX_WORKSHOP         1

/* Crafting rates */
#define CRAFT_IRON_BAR_TICKS   5   /* 2 iron ore -> 1 iron bar every 5 ticks */
#define CRAFT_IRON_ORE_COST    2
#define CRAFT_ALE_TICKS        3   /* 1 food + 1 wood -> 1 ale every 3 ticks */

/* Depth unlock costs */
#define DEPTH2_COST_STONE   500
#define DEPTH2_COST_GOLD    300
#define DEPTH3_COST_STONE  1500
#define DEPTH3_COST_GOLD   1000
#define DEPTH4_COST_STONE  3000
#define DEPTH4_COST_GOLD   2500
#define DEPTH5_COST_STONE  6000
#define DEPTH5_COST_GOLD   5000
#define DEPTH_MAX           5

/* Feed costs */
#define FEED_FOOD_COST        10
#define FEED_FATIGUE_RESTORE  20
#define FEED_MORALE_RESTORE   10

/* Hire costs */
#define HIRE_GOLD_BASE  50
#define HIRE_FOOD_BASE  20
#define HIRE_GOLD_COST  HIRE_GOLD_BASE
#define HIRE_FOOD_COST  HIRE_FOOD_BASE

/* =========================================================
 * Upgrade system
 * ========================================================= */
/* Upgrade IDs */
#define UPGR_PICK_QUALITY   0
#define UPGR_SAW_QUALITY    1
#define UPGR_IRRIGATION     2
#define UPGR_BARRACKS       3
#define UPGR_RECRUITERS     4
#define UPGR_WATCH_TOWER    5
#define UPGR_RUNE_HALLS     6
#define UPGR_MANA_WELL      7
#define UPGR_VAULT          8   /* gold cap */
#define UPGR_WAREHOUSE       9   /* stone+wood cap */
#define UPGR_GRANARY        10   /* food cap */
#define UPGR_WORKSHOP       11   /* unlocks Craftsdwarf */
#define UPGR_COUNT          12

/* Max levels per category */
#define UPGR_MAX_TOOLS      3
#define UPGR_MAX_WORKFORCE  3
#define UPGR_MAX_WATCHTOWER 3
#define UPGR_MAX_RUNEHALLS  5
#define UPGR_MAX_MANAWELL   3

#define UPGR_LEVEL(tier1, id)  (((tier1) >> ((id) * 4)) & 0xF)

/* Tool costs */
#define UPGR_COST_GOLD_TOOLS    150
#define UPGR_COST_STONE_TOOLS    75
#define UPGR_COST_WOOD_TOOLS     30
/* Workforce costs */
#define UPGR_COST_GOLD_WORK     200
#define UPGR_COST_STONE_WORK    100
/* Infrastructure costs */
#define UPGR_COST_GOLD_WATCH    200
#define UPGR_COST_STONE_WATCH   150
#define UPGR_COST_GOLD_RUNE     150
#define UPGR_COST_STONE_RUNE    100
#define UPGR_COST_MANA_RUNE      10   /* mana cost per level after lv1 */
#define UPGR_COST_GOLD_MANA     200
#define UPGR_COST_STONE_MANA     50

/* Workforce constants */
#define DWARF_CAP_BASE      16
#define DWARF_CAP_PER_LEVEL  8
#define HIRE_GOLD_DISCOUNT  10
#define HIRE_FOOD_DISCOUNT   5

/* Building degradation flags (GameState.flags bits) */
#define FLAG_WATCH_DEGRADED  (1 << 0)
#define FLAG_RUNE_DEGRADED   (1 << 1)
#define FLAG_MANA_DEGRADED   (1 << 2)

/* Job unlock checks */
#define GUARD_UNLOCKED(tier1)   (UPGR_LEVEL(tier1, UPGR_WATCH_TOWER) >= 1)
#define SCHOLAR_UNLOCKED(tier1) (UPGR_LEVEL(tier1, UPGR_RUNE_HALLS)  >= 1)

/* =========================================================
 * Research system — rune levels packed in Upgrades.tier2
 * bits [id*4 .. id*4+3] = stack count (0 = not researched)
 * ========================================================= */
#define RUNE_ENDURANCE   0   /* -1 fatigue rate/stack, max 5        */
#define RUNE_PLENTY      1   /* +5% yield/stack, max 5              */
#define RUNE_SWIFTNESS   2   /* +1 XP/tick all working/stack, max 5 */
#define RUNE_WARDING     3   /* -10% neg event severity/stack, max 3*/
#define RUNE_KINSHIP     4   /* +2 morale idle rate/stack, max 3    */
#define RUNE_DEEP        5   /* unlocks depth progression, max 3    */
#define RUNE_COUNT       6

#define RUNE_MAX_SMALL   5   /* Endurance, Plenty, Swiftness        */
#define RUNE_MAX_LARGE   3   /* Warding, Kinship, Deep              */

#define RUNE_COST_ENDURANCE  150
#define RUNE_COST_PLENTY     200
#define RUNE_COST_SWIFTNESS  175
#define RUNE_COST_WARDING    250
#define RUNE_COST_KINSHIP    200
#define RUNE_COST_DEEP       400

#define RUNE_LEVEL(tier2, id)  (((tier2) >> ((id) * 4)) & 0xF)

/* Requires Rune Halls Lv5 */
#define RESEARCH_UNLOCKED(tier1) (UPGR_LEVEL(tier1, UPGR_RUNE_HALLS) >= 5)

/* =========================================================
 * Prestige system — Clan Honor & Ancestor Tree
 * ========================================================= */

/* Node IDs — bitmask in PrestigeState.nodes */
/* Ancestor's Blessing (resource) */
#define PNODE_YIELD_1        0   /* +2% yield          cost 1 */
#define PNODE_YIELD_2        1   /* +4% yield          cost 2, req YIELD_1 */
#define PNODE_YIELD_3        2   /* +6% yield          cost 3, req YIELD_2 */
#define PNODE_START_GOLD_1   3   /* start +50 gold     cost 1, req YIELD_1 */
#define PNODE_START_GOLD_2   4   /* start +100 gold    cost 2, req START_GOLD_1 */
#define PNODE_START_STONE    5   /* start +50 stone    cost 1, req START_GOLD_1 */
#define PNODE_FOOD_TICK      6   /* +1 food/tick       cost 2, req START_GOLD_1 */
#define PNODE_WOOD_TICK      7   /* +1 wood/tick       cost 2, req FOOD_TICK */

/* Legendary Beards (workforce) */
#define PNODE_START_DWF_2    8   /* start 2 dwarves    cost 1 */
#define PNODE_START_DWF_3    9   /* start 3 dwarves    cost 2, req DWF_2 */
#define PNODE_START_DWF_4   10   /* start 4 dwarves    cost 3, req DWF_3 */
#define PNODE_HIRE_10       11   /* hire cost -10%     cost 1, req DWF_2 */
#define PNODE_HIRE_25       12   /* hire cost -25%     cost 2, req HIRE_10 */
#define PNODE_START_PICK    13   /* start Lv1 Pick     cost 1, req HIRE_10 */
#define PNODE_START_SAW     14   /* start Lv1 Saw      cost 1, req START_PICK */
#define PNODE_START_IRR     15   /* start Lv1 Irr.     cost 1, req START_SAW */
#define PNODE_START_BARR    16   /* start Barracks Lv1 cost 2, req DWF_3 */

/* Clan Reputation (research) */
#define PNODE_RUNE_10       17   /* rune cost -10%     cost 1 */
#define PNODE_RUNE_25       18   /* rune cost -25%     cost 2, req RUNE_10 */
#define PNODE_START_MANA    19   /* start +100 mana    cost 1, req RUNE_10 */
#define PNODE_START_RUNE_H  20   /* start Rune Halls 1 cost 2, req START_MANA */
#define PNODE_MANA_TICK_1   21   /* +1 mana/tick       cost 2, req START_MANA */
#define PNODE_MANA_TICK_2   22   /* +2 mana/tick       cost 3, req MANA_TICK_1 */
#define PNODE_START_SCHOLAR 23   /* Scholar from start cost 2, req START_RUNE_H */
#define PNODE_COUNT         24

#define PNODE_BIT(n)        (1ULL << (n))
#define PNODE_UNLOCKED(nodes, n) (((nodes) >> (n)) & 1)

/* Prestige requirements */
#define PRESTIGE_MIN_TICKS      1000
#define PRESTIGE_MIN_RAIDS      1
#define PRESTIGE_MIN_RUNES      3
#define PRESTIGE_MIN_RESOURCES  10000

typedef struct {
    uint32_t honor;             /* unspent honor                    */
    uint32_t total_honor;       /* all-time honor earned            */
    uint32_t total_prestiges;   /* number of times prestiged        */
    uint32_t _pad;
    uint64_t nodes;             /* bitmask of purchased nodes       */
    uint64_t total_resources;   /* lifetime resources for honor calc*/
} PrestigeState;

/* =========================================================
 * Raid / Breach system
 * ========================================================= */
#define RAID_NONE       0   /* no raid active              */
#define RAID_WARNING    1   /* warning phase               */
#define RAID_COMBAT     2   /* combat in progress          */
#define RAID_RESULT     3   /* resolved, awaiting cleanup  */

#define RAID_MAX_GUARDS 8
#define RAID_WARN_TICKS 60  /* ticks between warning and combat */
#define RAID_FIRST_TICK 300 /* tick of first raid warning  */
#define RAID_INTERVAL   500 /* ticks between raids         */
#define RAID_COMBAT_INTERVAL 5 /* ticks between damage rounds */
#define RAID_TIMER      300 /* ticks before raid auto-resolves as loss */

typedef struct {
    uint8_t  active;                    /* RAID_* state            */
    uint8_t  threat;                    /* 1-5                     */
    uint8_t  guard_count;               /* guards in raid          */
    uint8_t  _pad;
    int32_t  enemy_hp;
    int32_t  enemy_hp_max;
    int32_t  enemy_atk;
    uint8_t  guard_idx[RAID_MAX_GUARDS];
    int32_t  guard_hp[RAID_MAX_GUARDS];
    int32_t  guard_hp_max[RAID_MAX_GUARDS];
    uint64_t next_raid_tick;            /* when next warning fires */
    uint64_t combat_start_tick;         /* when combat began       */
    uint64_t last_combat_tick;          /* last damage exchange    */
    int32_t  reward_gold;               /* gold on win             */
    int32_t  raids_completed;           /* for threat scaling      */
} Raid;                                 /* size: ~120 bytes        */

typedef struct { uint64_t tier1, tier2; } Upgrades;
typedef struct { uint64_t seed; } RngState;

typedef struct {
    uint8_t code, severity, dwarf_idx;
    uint8_t _pad[5];
} PendingEvent;

#define EVT_FLAVOUR     0
#define EVT_POSITIVE    1
#define EVT_NEGATIVE    2
#define EVT_MILESTONE   3
#define EVENT_LOG_SIZE  64
#define EVENT_INTERVAL  30

typedef struct {
    uint64_t tick;
    uint8_t  code, severity, dwarf_idx;
    uint8_t  _pad[5];
} EventRecord;

typedef struct {
    EventRecord entries[EVENT_LOG_SIZE];
    uint8_t     head, count;
    uint8_t     _pad[6];
} EventLog;

typedef struct {
    /* offset 0x0000 */ Resources    resources;
    /* offset 0x0028 */ Dwarf        dwarves[MAX_DWARVES];
    /* offset 0x1028 */ Upgrades     upgrades;
    CraftSlot    craft[RECIPE_COUNT];   /* 6 * 4 = 24 bytes */
    /* offset 0x1038 */ RngState     rng;
    /* offset 0x1040 */ uint64_t     tick;
    /* offset 0x1048 */ uint32_t     depth;
    /* offset 0x104C */ uint32_t     depth_xp;   /* progress toward next depth */
    /* offset 0x104C */ uint32_t     flags;
    /* offset 0x1050 */ PendingEvent pending;
    /* offset 0x1058 */ EventLog     event_log;
    /* offset 0x1060 */ Raid         raid;
    PrestigeState prestige;
} GameState;

extern void     asm_tick(GameState *state);
extern void     asm_rng_seed(GameState *state, uint64_t seed);
extern uint64_t asm_rng_next(GameState *state);
extern void     asm_event_push(GameState *state, uint8_t code,
                               uint8_t severity, uint8_t dwarf_idx);
extern void     asm_tick_dwarves(GameState *state);
extern int64_t  asm_hire_dwarf(GameState *state);
extern int64_t  asm_assign_job(GameState *state, uint8_t dwarf_idx,
                               uint8_t job);
extern const char *asm_get_dwarf_name(uint8_t idx);
extern void     asm_tick_breach(GameState *state);
extern int64_t  asm_buy_pnode(GameState *state, uint8_t node_id);
extern int64_t  asm_do_prestige(GameState *state);
extern int64_t  asm_can_prestige(GameState *state);
extern void     asm_breach_retreat(GameState *state);
extern int64_t  asm_dig_deeper(GameState *state);
extern int64_t  asm_craft_assign(GameState *state, uint8_t recipe_id, int delta);
extern void     asm_tick_craft(GameState *state);
extern int64_t  asm_feed_dwarf(GameState *state, uint8_t dwarf_idx);
extern int64_t  asm_buy_rune(GameState *state, uint8_t rune_id);
extern int64_t  asm_buy_upgrade(GameState *state, uint8_t upgrade_id);
extern uint64_t asm_state_checksum(const void *data, uint64_t len);
extern int64_t  asm_save_game(const char *path, const GameState *state);
extern int64_t  asm_load_game(const char *path, GameState *state);

#endif /* ASMORIA_H */