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
%define DWARF_XP            0x08
%define SIZEOF_DWARF        0x10
%define MAX_DWARVES         64

%define JOB_IDLE            0
%define JOB_MINER           1
%define JOB_LUMBERER        2
%define JOB_FARMER          3
%define JOB_GUARD           4
%define JOB_SCHOLAR         5

%define FATIGUE_MAX         100
%define FATIGUE_WARN        70
%define MORALE_MAX          100

%define GS_RESOURCES        0x0000
%define GS_DWARVES          0x0028
%define GS_UPGRADES         0x0428
%define GS_RNG_SEED         0x0438
%define GS_TICK             0x0440
%define GS_DEPTH            0x0448
%define GS_FLAGS            0x044C
%define GS_PENDING          0x0450
%define GS_EVENTLOG         0x0458

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

%define EVENT_INTERVAL      15