# ASMoria

> *"Not all who delve deep find only stone."*

A dwarf fortress city-builder written in **C/SDL2** with all game logic in **x86-64 NASM assembly**. Manage your workforce, research ancient runes, defend against raids, and build a legacy worth carving into stone.

---

## Features

- **Economy** — Gold, Stone, Wood, Food, Mana. Resource generation scales with job level, morale, upgrades, and runes. Buildings require ongoing upkeep.
- **Dwarves** — Up to 64 named dwarves across 6 jobs (Miner, Lumberer, Farmer, Guard, Scholar, Idle). Per-job XP and levels, morale/fatigue cycles, food healing.
- **Upgrades** — Tools, Workforce, and Infrastructure upgrades across three categories. Watch Tower unlocks Guards, Rune Halls unlocks Scholars and Research, Mana Well generates passive mana.
- **Research** — Six ancient runes inscribed by Scholars at scaling mana cost (Endurance, Plenty, Swiftness, Warding, Kinship, Deep).
- **The Breach** — When a raid fires, assign guards and fight back. ASCII sprite enemies, threat scaling 1–5, win/lose/retreat outcomes, flashing red border during combat.
- **Prestige** — Seal the chronicles and begin anew. Earn Clan Honor and spend it on 24 permanent nodes across three branches (Ancestor's Blessing, Legendary Beards, Clan Reputation).

---

## Building

```bash
# Dependencies (Ubuntu / Debian)
sudo apt install nasm gcc libsdl2-dev libsdl2-ttf-dev

# Build and run
make run
```

---

## Controls

| Key | Action |
|---|---|
| `↑` / `↓` | Select dwarf |
| `←` / `→` | Scroll dwarf list |
| `M L F G S I` | Assign job |
| `H` | Hire dwarf |
| `E` | Feed selected dwarf |
| `U` / `R` / `B` / `P` | Upgrades / Research / Breach / Prestige |
| `PgUp` / `PgDn` | Scroll event log |
| `F5` / `F9` | Save / Load |
| `ESC` | Close panel / Quit |

---

## Architecture

Strict separation between presentation (C/SDL2) and logic (NASM):

- All game logic lives in `asm/core/` — resources, morale, XP, combat, prestige, save/load
- `GameState` is a flat C struct; ASM modules access it via verified offsets from `offsets.inc`
- File I/O uses raw Linux syscalls with FNV-1a checksum — no libc
- The C layer handles only SDL2 rendering and input routing

**Platform:** Linux x86-64 (WSL2 compatible) · **Tick rate:** 500ms · **Window:** 1280×800

---

## Roadmap

See [`ROADMAP.md`](Doc/Roadmap.md) for the 6-week plan and long-term vision.
See [`Delivery_to_do_list.md`](Doc/Delivery_to_do_list.md) for the current checklist.