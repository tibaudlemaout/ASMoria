# ASMoria — Development Roadmap
### 6-Week Plan (Post-Core Milestone)

---

## Current State (Week 0 — Baseline)

The core game loop is complete and functional:

- **Economy** — Resource generation (gold, stone, wood, food, mana) with morale/fatigue scaling, upgrade bonuses, and job levels
- **Dwarves** — 64 named dwarves, 6 jobs, XP/level system, food healing, keyboard navigation
- **Upgrades** — Tools, Workforce, Infrastructure (Watch Tower, Rune Halls, Mana Well) with wood/stone/mana upkeep
- **Research** — 6 runes with scaling mana costs, Scholar job, Rune Halls progression
- **Combat** — Guard job, Breach system with ASCII sprites, threat scaling, retreat/win/loss outcomes
- **Prestige** — Clan Honor tree (24 nodes across 3 branches), run reset with permanent bonuses
- **Save/Load** — Raw Linux syscall I/O with FNV-1a checksum, auto-save on quit

---

## Week 1 — Depth & Mining Layers

**Goal:** Make the underground feel like a real place worth exploring.

### Mining Depth System
- Unlock deeper layers by spending accumulated resources (stone + gold + manpower)
- Each depth level increases yield multiplier but also increases fatigue rates
- Depth displayed in title bar (currently stuck at 1)
- Rune of the Deep (already in research tree) gates progression — max depth = rune stacks + 1

### Resource Tiers by Depth
| Depth | New Resources | Yield Bonus | Risk |
|---|---|---|---|
| 1 (Surface) | Stone, Gold, Wood, Food | Base | None |
| 2 (Caverns) | Iron Ore | +20% | +1 fatigue/tick miners |
| 3 (Deep Veins) | Gems | +40% | Raid frequency +25% |
| 4 (Ancient Ruins) | Relics | +60% | Rare ruin events |
| 5 (Magma Zone) | Crystals | +80% | +2 fatigue/tick all |

### Implementation Notes
- `depth` field already exists in `GameState`
- New `asm_dig_deeper(state)` function in ASM
- Depth-gated events added to `events.asm` and `events_text.h`
- UI: new depth progress bar in title bar area

---

## Week 2 — Storage & Crafting Foundation

**Goal:** Add resource pressure and production chains to prevent snowballing.

### Storage Capacity
- Each resource type has a cap based on built storage structures
- Default caps: Gold 500, Stone 500, Wood 200, Food 200, Mana 100
- Build Granary (food), Vault (gold), Warehouse (stone/wood) to increase caps
- Resources stop generating when at cap — creates natural pressure to spend

### Basic Crafting
- New resource types: Iron Bars (from Iron Ore), Ale (from Food + Wood), Tools
- Crafting assigned to idle dwarves as a new job: **Craftsdwarf**
- Simple conversion ratios: 2 Iron Ore → 1 Iron Bar per 10 ticks
- Crafted goods used as upgrade costs for higher-tier buildings

### Great Forge (Foundation)
- New infrastructure upgrade unlocked at depth 3
- Accepts raw materials, produces crafted goods passively
- Foundation for Week 5 endgame system

---

## Week 3 — Tavern & Morale Expansion

**Goal:** Give morale real weight and create interesting economic tradeoffs.

### Tavern System
- New building: Tavern (requires Wood + Stone to build)
- Produces Ale (consumes Food + Wood each tick)
- Dwarves consume Ale passively — each unit consumed gives +5 morale
- Without Ale, morale decay accelerates for non-idle dwarves

### Morale Effects Expansion
Current morale only affects yield. Expand to:
- **Combat:** Guard HP scales with morale (already partially implemented)
- **Research:** Scholar XP gain scales with morale
- **Events:** Low morale triggers new negative event pool (strikes, accidents)
- **Fatigue:** High morale (>90) reduces fatigue rate by 1

### Housing System
- Dwarves need housing — Barracks provides beds
- Overcrowded dwarves (more alive than bed capacity) get -1 morale/tick
- Upgradeable dormitories beyond current Barracks system

---

## Week 4 — Hero Units & Combat Expansion

**Goal:** Make the Breach system feel more dynamic and give guards identity.

### Hero Dwarves
- Rare chance on hire (5%) to get a Hero dwarf with a unique trait
- Traits are permanent and visible in the dwarf panel
- Example traits:
  - **Ironhide** — +50% Guard HP
  - **Berserker** — +100% Guard ATK, -30 morale
  - **Scholar-King** — +2 mana/tick while Scholar
  - **Foreman** — adjacent dwarves get +1 XP/tick

### Raid Variety
- Different raid types beyond generic threat levels:
  - **Goblin Horde** — many weak enemies, fast
  - **Troll Siege** — single high-HP enemy, slow but devastating
  - **Necromancer** — killed guards come back as enemies
  - **Dragon Sighting** — warning only, resource drain if not repelled
- Raid type determined by depth level + RNG

### Trap Engineering
- New upgrade category: Traps
- Spike Pit, Flame Vent, Ballista
- Each trap deals automatic damage per combat round
- Costs Wood + Stone to build, degrades over time (upkeep)

---

## Week 5 — Economy, Trade & Automation

**Goal:** Give experienced players optimization tools and late-game sinks.

### Caravan Trading
- Send a caravan (costs 5 food/tick while travelling, 50 ticks round trip)
- Trade resources for gold or rare blueprints
- Risk of ambush — guards can be assigned to escort
- Dynamic pricing: surplus resources sell for less

### Foreman System
- New late-game upgrade: Foreman
- Automates job assignment based on configurable rules:
  - "If food < 100, assign 2 dwarves to Farmer"
  - "If raid warning, assign all guards"
- Reduces micro-management in late game

### Blueprint Presets
- Save current job assignments as a named preset
- Quick-switch between Economy / Defense / Research modes
- Stored in save file, up to 4 presets

---

## Week 6 — Endgame, Polish & Ascension

**Goal:** Provide long-term goals and prepare for post-launch content.

### World Wonders
Massive multi-stage construction projects, each requiring hundreds of ticks and thousands of resources:
- **Throne of the Mountain King** — requires depth 5, all runes maxed, 5000 stone + 2000 gold
- **World Drill** — requires depth 4 + Rune of the Deep maxed, unlocks Ascension
- **Divine Hammer** — requires Great Forge + all crafting chains, doubles all yields permanently

### Ascension — New Mountain Biomes
After completing a World Wonder, unlock a new run biome:
- **Frozen Peak** — resources frozen, need Ale to keep dwarves working; unique Ice Crystal resource
- **Volcano Core** — massive yields but constant fire damage; fire-resistant runes needed
- **Haunted Range** — undead raid variants; morale constantly drains; Warding rune critical
- **Golden Mountain** — all gold yields tripled but no stone; must trade for everything

### Balance & Polish
- Comprehensive balance pass across all systems
- Additional event text (target: 128 unique events, currently ~64)
- Sound effect hooks (SDL_mixer integration)
- High score / achievements screen
- In-game help panel (? key) with system explanations

---

## Priority Matrix

| System | Impact | Effort | Week |
|---|---|---|---|
| Mining Depth | Very High | Medium | 1 |
| Storage Caps | High | Low | 2 |
| Crafting Chains | High | High | 2 |
| Tavern / Ale | Medium | Medium | 3 |
| Morale Expansion | High | Low | 3 |
| Hero Dwarves | High | Medium | 4 |
| Raid Variety | Medium | Medium | 4 |
| Traps | Medium | High | 4 |
| Caravan Trade | High | High | 5 |
| Foreman System | Medium | High | 5 |
| World Wonders | Very High | High | 6 |
| Ascension Biomes | Very High | Very High | 6 |

---

## Technical Debt & Infrastructure

Items to address in parallel with feature work:

- **`offsets.inc` maintenance** — verify struct offsets with `offsetof()` after every `GameState` change
- **Event text expansion** — `events_text.h` needs more variety to avoid repetition at 64+ dwarves
- **Encoding safety** — `strncpy` truncation warnings need proper `strnlen` guards
- **`SAVE_VERSION` discipline** — bump on every `GameState` structural change; consider auto-deriving from `sizeof`
- **Stack alignment audit** — any new ASM function with calls needs the 6-push + `sub rsp,8` pattern verified
- **PIE compliance** — all `.data` refs must use `[rel label]`; new ASM files need review

---

## Out of Scope (Post-6-Week)

These are strong ideas that would require significant architecture work:

- **Multiplayer / Clan Wars** — requires networking layer
- **Procedural dungeon maps** — requires spatial data structures
- **Visual tile renderer** — would replace the text grid entirely
- **Mobile port** — SDL2 touch input layer needed
- **Mod support** — requires scripting layer (Lua integration)

---

*ASMoria is built in C/SDL2 with x86-64 NASM assembly for all game logic. Target platform: Ubuntu Linux (WSL2 compatible).*