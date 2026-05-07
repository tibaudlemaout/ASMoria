# ASMoria — Development Roadmap

> **Note on scope:** This document is split into two parts.
> **Part 1** covers the committed 6-week delivery plan with concrete, buildable features.
> **Part 2** is a long-term vision document — ideas, ambitions, and design directions
> that go well beyond the 6-week window. Nothing in Part 2 is a delivery commitment.
> It exists to show where the game *could* go given continued development time.

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

# PART 1 — 6-Week Delivery Plan

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
- UI: depth progress bar and dig button in title bar area

---

## Week 2 — Storage & Crafting Foundation

**Goal:** Add resource pressure and production chains to prevent snowballing.

### Storage Capacity
- Each resource type has a cap based on built storage structures
- Default caps: Gold 500, Stone 500, Wood 200, Food 200, Mana 100
- Build Granary (food), Vault (gold), Warehouse (stone/wood) to increase caps
- Resources stop generating when at cap — creates natural pressure to spend

### Basic Crafting
- New resource types: Iron Bars (from Iron Ore), Ale (from Food + Wood)
- Crafting assigned to idle dwarves as a new job: **Craftsdwarf**
- Simple conversion ratios: 2 Iron Ore → 1 Iron Bar per 10 ticks
- Crafted goods used as upgrade costs for higher-tier buildings

### Great Forge (Foundation)
- New infrastructure upgrade unlocked at depth 3
- Accepts raw materials, produces crafted goods passively
- Foundation for Week 6 endgame system

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
- Rare chance on hire (5%) to get a Hero dwarf with a unique permanent trait
- Traits visible in the dwarf panel with a distinct colour
- Example traits:
  - **Ironhide** — +50% Guard HP
  - **Berserker** — +100% Guard ATK, -30 morale
  - **Scholar-King** — +2 mana/tick while Scholar
  - **Foreman** — nearby dwarves get +1 XP/tick

### Raid Variety
- Different raid types beyond generic threat levels:
  - **Goblin Horde** — many weak enemies, fast
  - **Troll Siege** — single high-HP enemy, slow but devastating
  - **Necromancer** — KO'd guards temporarily fight for the enemy
  - **Dragon Sighting** — warning only, resource drain if not repelled
- Raid type determined by depth level + RNG

### Trap Engineering
- New upgrade category: Traps
- Spike Pit, Flame Vent, Ballista — each deals passive damage per combat round
- Costs Wood + Stone to build, requires upkeep to maintain

---

## Week 5 — Economy, Trade & Automation

**Goal:** Give experienced players optimisation tools and late-game resource sinks.

### Caravan Trading
- Send a caravan (costs 5 food/tick while travelling, 50 ticks round trip)
- Trade resources for gold or rare blueprints
- Risk of ambush — guards can be assigned to escort
- Dynamic pricing: surplus resources sell for less

### Foreman System
- New late-game upgrade unlocking the Foreman role
- Automates job assignment based on configurable thresholds:
  - "If food < 100, assign 2 dwarves to Farmer"
  - "If raid warning active, assign all idle dwarves to Guard"
- Reduces micro-management in late game

### Blueprint Presets
- Save current job assignments as a named preset (up to 4)
- Quick-switch between Economy / Defense / Research modes
- Stored in save file

---

## Week 6 — Endgame, Polish & Ascension

**Goal:** Provide long-term goals and prepare the game for continued development.

### World Wonders
Massive multi-stage construction projects:
- **Throne of the Mountain King** — depth 5 + all runes maxed + 5000 stone + 2000 gold
- **World Drill** — depth 4 + Rune of the Deep maxed, unlocks Ascension
- **Divine Hammer** — Great Forge + all crafting chains, doubles all yields permanently

### Ascension — New Mountain Biomes
After completing a World Wonder, unlock a new run biome with unique rules:
- **Frozen Peak** — resources frozen without Ale; unique Ice Crystal resource
- **Volcano Core** — massive yields but constant fire damage
- **Haunted Range** — undead raid variants; morale constantly drains
- **Golden Mountain** — gold yields tripled but no stone; everything must be traded

### Balance & Polish
- Comprehensive balance pass across all systems with play-testing
- Event text expansion (target: 128 unique events, currently ~64)
- Sound effect hooks (SDL_mixer integration)
- In-game help panel (? key) with system explanations
- High score and achievements screen

---

## 6-Week Priority Matrix

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

## Technical Debt (Ongoing Throughout)

- **`offsets.inc` maintenance** — verify struct offsets with `offsetof()` after every `GameState` change
- **Event text expansion** — `events_text.h` needs more variety to avoid repetition at 64+ dwarves
- **Encoding safety** — `strncpy` truncation warnings need proper `strnlen` guards
- **`SAVE_VERSION` discipline** — bump on every `GameState` structural change
- **Stack alignment audit** — any new ASM function with calls needs the 6-push + `sub rsp,8` pattern verified
- **PIE compliance** — all `.data` refs must use `[rel label]`; new ASM files need review

---

---

# PART 2 — Long-Term Vision

> ⚠️ **Nothing below this line is a delivery commitment.**
> These are design explorations and creative directions for ASMoria beyond the 6-week window.
> They are included to illustrate the depth of the design space and to inform
> longer-term planning if development continues.

---

## The Deep Lore Layer

### Runic Language System
Scholars don't just research runes — they *decipher* them. Each rune has a translation progress that fills over time. Fully translated runes unlock lore entries and bonus effects. Some runes are contradictory — translating both locks one out permanently. Creates genuine narrative choices and a sense of uncovering an ancient civilization buried beneath your fortress.

### The Book of Grudges
A dwarven staple. Every negative event, every raid loss, every dwarf knocked out gets recorded as a Grudge. Grudges can be "settled" by specific actions — repel a raid, mine a threshold, reach a depth milestone. Settling gives honor and rare bonuses. Leaving grudges unsettled triggers escalating consequences. A quest system that feels completely dwarven without needing a separate quest UI.

### Ancestor Spirits
After enough prestiges, specific dwarves from previous runs appear as Ancestor Spirits — passive advisors with unique event text referencing things they accomplished. *"Gotrek's spirit watches approvingly as you strike a rich vein."* Makes each run feel connected to the ones before it and rewards long-term play.

### The Saga
An auto-generated chronicle of each run written in dwarven saga style. Key events are recorded as verse — first raid, first rune, first hero, deaths. At prestige the saga is sealed and stored. Past sagas can be read in a dedicated panel. Pure flavour that rewards the player's history with the game.

---

## The Political Layer

### Dwarf Council & Guild Factions
At a certain population threshold, dwarves organise into factions — Miners Guild, Brewers Guild, Warriors Guild, Scholars Guild. The dominant guild influences which upgrades are cheaper, which events fire more often, and what passive bonus the fortress holds. Managing guild influence becomes a meta-game within the run, with interesting tradeoffs as factions compete for dominance.

### Noble Visitors
Periodically a Noble arrives from the surface — a merchant prince, a general, an archmage. Each has a demand and an offer. Accept their demand (divert farmers for 200 ticks) and receive their reward (rare blueprint, unique dwarf). Refuse and face consequences (trade embargo). Creates short-term vs long-term decisions with diplomatic flavour.

### Succession Crisis
When the fortress reaches sufficient size, one dwarf is designated Thane. When the Thane retires, a succession event fires — multiple candidates compete, each backed by a guild faction. The choice of successor determines which guild receives a permanent bonus for the remainder of the run. High drama, very dwarven.

---

## The Combat Layer (Expanded)

### Siege Warfare
Late-game raids escalate into proper sieges. Enemies bring siege equipment — battering rams against gates, catapults that damage buildings directly, sappers who undermine walls. The player responds with counter-siege options: boiling oil (consumes food), ballista fire (consumes wood), sortie attacks (high risk/reward). Transforms the Breach from a passive wait into an active defence puzzle.

### Dungeon Bosses
At specific depth thresholds, a Boss creature guards further progression. Unlike raids, bosses don't come to you — you send an expedition party into the deep. Party composition matters: guards for HP, scholars for ability identification, miners for environmental advantages. A lost expedition means the party is KO'd for 300 ticks and the depth remains locked.

### The Grudge War
If grudges accumulate past a threshold, a Grudge War triggers — a prolonged campaign of escalating raids over 20 in-game days. Retreat and negotiation are unavailable. Fight through all waves or surrender the fortress with a significant honor penalty. Brutal by design, but creates genuinely memorable sessions.

### Mercenary Contracts
Hire surface mercenaries for a gold-per-tick cost. They are powerful but unreliable — their morale doesn't respond to the regular systems, they can desert if unpaid, and they occasionally cause incidents with regular dwarves. Valuable in a crisis, dangerous as a long-term strategy.

### Trap Engineering (Advanced)
Beyond basic traps — combinations trigger chain reactions. Spike Pit + Oil Slick produces a fire trap. Ballista + Rune Crystal fires a runic bolt. Discovering and building combinations rewards experimentation and creates a distinct "defence engineering" gameplay strand.

---

## The Production Layer (Expanded)

### Full Supply Chains
Resources flow through actual production stages, each requiring an assigned dwarf:
```
Iron Ore → Smelter (Craftsdwarf) → Iron Bars
Iron Bars + Coal → Forge (Smith) → Steel
Steel + Handles → Armory (Armorer) → Weapons
Weapons → Garrison (Guard) → Combat Bonus
```
Bottlenecks in the chain become interesting optimisation puzzles — the kind that idle games do well.

### Resource Decay
Food spoils without a Granary. Ale goes flat without a Cellar. Iron rusts without oil. Creates ongoing pressure to maintain infrastructure rather than accumulate indefinitely — complementing the storage cap system from Week 2.

### The Black Market
A shady merchant appears periodically with suspiciously good trades. Occasionally it is a trap: resources are cursed (negative events for 500 ticks) or the merchant was a spy and the next raid exploits specific weaknesses. Risk/reward that feels alive rather than scripted.

---

## The Exploration Layer

### Expedition System
Send a party of dwarves on a surface expedition — gone for N ticks, returning with exotic resources, blueprints, or new recruits. Longer expeditions yield better rewards at higher risk. Party composition affects outcomes: miners find ore caches, scholars find ancient texts, guards survive ambushes.

### The Underdark Map
A procedurally generated underground map that unfolds as you dig. Each tile can contain a resource node, a dungeon room, a boss lair, a buried ruin, or a hazard. Miners explore adjacent tiles passively each tick. Scholars survey tiles before digging, revealing contents at mana cost. Introduces spatial strategy to the otherwise time-based game.

### Rival Fortress
Another dwarven clan digs toward the same deep resources. A race mechanic — whoever reaches depth 5 first gets permanent access to the richest veins. The rival's progress is visible on the map. Sabotage is possible (expensive and risky) or the player can race honestly. Adds competitive tension to solo play.

### Lost Expeditions
Occasionally a message arrives — a previous expedition party never returned. Send a rescue team into a procedurally described situation. Text-based branching choices with mechanical consequences. Outcomes depend on party composition and the choices made. Some parties are never found.

---

## The Meta Layer

### Procedural Dwarf Personalities
Each dwarf gets 2 randomly assigned personality traits at hire:

| Trait | Effect |
|---|---|
| Stubborn | +20% morale recovery, refuses reassignment 20% of the time |
| Greedy | +10% gold yield, costs 2x food |
| Brave | Guard HP +30%, volunteers first for raids |
| Scholarly | +2 XP/tick all jobs, costs more to hire |
| Clumsy | -10% yield but immune to negative fatigue events |
| Ancient Blood | Rare — random powerful bonus unique to each instance |

Makes every dwarf feel like a character rather than a unit and rewards paying attention to your roster.

### Dwarf Relationships
Dwarves who work together long enough become friends — gaining small mutual bonuses when assigned to the same job. Dwarves who are incompatible suffer morale penalties when near each other. Managing the social dynamics of your workforce adds a light RPG layer to the city builder.

### Clan Lineage
Each prestige generates a generation of clan history. A family tree builds up over multiple prestiges — recording which generation first reached each depth, built the first Rune Halls, repelled the most raids. Pure flavour but deeply satisfying for long-term players.

### Challenge Runs
Named modifiers for harder or stranger playthroughs:

| Challenge | Rule |
|---|---|
| Oathbound | Every dwarf refuses to change jobs once assigned |
| The Curse | One random resource generates 0 and must be traded for |
| Iron Dwarf | Permadeath — KO'd guards die permanently |
| Ancestors' Test | No prestige nodes active; scored by total resources |
| The Siege | A raid fires every 200 ticks from the start |
| One Beard | Maximum 1 dwarf at all times |

### Seasonal Events
Time-limited modifiers tied to the real-world calendar:
- **Durin's Day** (winter solstice) — mining yields doubled for 24 real-world hours
- **The Long Winter** (January) — food consumption doubled, morale decays faster
- **Festival of Axes** (midsummer) — guards gain XP 5x faster

---

## Endgame Vision

### The Mythril Throne
The true endgame objective. Mythril exists only at depth 5 in tiny quantities. Building the Mythril Throne requires 500 mythril, all World Wonders completed, and 10 prestiges minimum. The Throne doesn't end the game — it transforms it. The fortress becomes a Dwarven Kingdom, unlocking diplomacy with surface nations, tribute from vassal mines, and a king's court with its own political mechanics.

### The World Core
At depth 10 (requires the World Drill wonder to even attempt), the player reaches the literal core of the world. This is not a raid — it is a slow escalating corruption that spreads upward through the fortress, transforming resources into void matter and turning dwarves hostile. Winning unlocks a permanent "Core Fragment" bonus visible in all future runs. Losing is spectacular and permanent.

### Dynamic Difficulty Scaling
The game tracks a hidden "fortress power score" and adjusts raid difficulty, event frequency, and resource decay rates to match. A casual player faces gentle raids; a highly optimised fortress faces genuine existential threats. No explicit difficulty setting — the game reads the player and responds accordingly.

### The Runic Computer
A late-game wonder that is literally a mechanical computing device built inside the mountain from crafted components. Once built, it optimises job assignments automatically based on current resource levels, identifies the most efficient upgrade path, and predicts upcoming raid types based on depth and raid history. Enormous investment to construct but effectively solves the micro-management problem for players who want pure idle progression.

---

## Technical Ambitions

These require architectural work beyond incremental feature development:

- **Procedural dungeon maps** — spatial data structures for the Underdark map system
- **SDL_mixer integration** — ambient cave sounds, combat effects, event musical stings
- **Optional graphical tile mode** — SDL2 texture renderer running alongside the text grid, toggled at runtime
- **Mod support** — Lua scripting layer for community-created events, runes, and raid types
- **Mobile port** — SDL2 touch input layer for Android/iOS; the text grid scales well to small screens
- **Steam integration** — achievements, cloud saves, leaderboards, trading cards

---

## Out of Scope (Fundamental Architecture Changes)

These are genuinely interesting ideas that would require rebuilding significant parts of the engine:

- **Full 3D dungeon view** — would require replacing the text renderer entirely with a 3D pipeline
- **Real-time multiplayer** — requires deterministic simulation and a networking layer
- **Procedural music generation** — requires an audio synthesis engine, not just sample playback
- **Competitive multiplayer** — clan-vs-clan fortress raids requiring persistent server infrastructure

---

*ASMoria is built in C/SDL2 with x86-64 NASM assembly for all game logic.*
*Target platform: Ubuntu Linux (WSL2 compatible).*
*Architecture designed for long-term extensibility — new systems slot in as ASM modules*
*with C UI layers, keeping the core loop stable as features grow.*