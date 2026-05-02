%define RES_GOLD            0x00
%define RES_STONE           0x08
%define RES_WOOD            0x10
%define RES_FOOD            0x18
%define RES_MANA            0x20
%define SIZEOF_RESOURCES    0x28

%define DWARF_ALIVE         0x00
%define DWARF_JOB           0x01
%define DWARF_MORALE        0x02
%define DWARF_FATIGUE       0x03
%define DWARF_PREV_JOB      0x04
%define DWARF_JOB_LEVEL     0x05
%define DWARF_NAME_IDX      0x0B    ; uint8_t — index into name table
%define DWARF_JOB_XP        0x10
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
%define MORALE_IDLE_RATE    10

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

%define XP_LVL2             500
%define XP_LVL3             1500
%define XP_LVL4             3500
%define XP_LVL5             7500

%define XP_RATE_IDLE        2
%define XP_RATE_MINER       3
%define XP_RATE_LUMBERER    3
%define XP_RATE_FARMER      2
%define XP_RATE_GUARD       2
%define XP_RATE_SCHOLAR     2

%define UPGR_PICK_QUALITY   0
%define UPGR_SAW_QUALITY    1
%define UPGR_IRRIGATION     2
%define UPGR_BARRACKS       3
%define UPGR_RECRUITERS     4
%define UPGR_COUNT          5
%define UPGR_MAX_TOOLS      3
%define UPGR_MAX_WORKFORCE  3
%define UPGR_COST_GOLD_TOOLS    100
%define UPGR_COST_STONE_TOOLS    50
%define UPGR_COST_GOLD_WORK     150
%define UPGR_COST_STONE_WORK     75
%define DWARF_CAP_BASE      16
%define DWARF_CAP_PER_LEVEL 16
%define HIRE_GOLD_BASE      50
%define HIRE_FOOD_BASE      20
%define HIRE_GOLD_DISCOUNT  10
%define HIRE_FOOD_DISCOUNT   5

%define GS_SIZE             5216