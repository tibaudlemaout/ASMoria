# ASMoria — 6-Week Progress Tracker

---

## ✅ Core (Complete)

- [x] Project skeleton, Makefile, ASM build system
- [x] xorshift64 RNG
- [x] Resource tick (gold, stone, wood, food, mana)
- [x] Morale & fatigue system
- [x] 6 jobs (Idle, Miner, Lumberer, Farmer, Guard, Scholar)
- [x] Per-job XP & level system
- [x] 64 named dwarves (lore references)
- [x] Hire system with scaling cost
- [x] Tools, Workforce & Infrastructure upgrades
- [x] Building upkeep (wood / mana / stone)
- [x] Research runes (6 runes, scaling mana cost)
- [x] Food healing
- [x] Guard Breach system (ASCII sprites, threat scaling, win/loss/retreat)
- [x] Prestige system (24-node Clan Honor tree)
- [x] Save / Load (raw syscalls, FNV-1a checksum)
- [x] Event log (64 events, word-wrap, scroll)
- [x] Balance pass

---

## Week 1 — Depth & Mining Layers

- [x] `asm_dig_deeper()` function
- [x] Depth unlock cost (stone + gold)
- [x] Depth yield multiplier per level
- [x] Depth fatigue penalty per level
- [x] New resources per depth (Iron Ore, Gems, Relics, Crystals)
- [x] Depth-gated events in `events_text.h`
- [x] UI: depth progress bar + dig button in title bar
- [x] Rune of the Deep gates max depth

---

## Week 2 — Storage & Crafting

- [x] Resource storage caps (per type)
- [x] Granary, Vault, Warehouse upgrades
- [x] Craftsdwarf job
- [x] Iron Bars crafting (2 ore → 1 bar / 10 ticks)
- [x] Ale crafting (food + wood)
- [x] Crafted goods as upgrade costs
- [x] Great Forge building (depth 3 unlock)
- [x] UI: storage cap display per resource

---

## Week 3 — Tavern & Morale Expansion

- [x] Tavern building (wood + stone)
- [x] Ale consumption → morale bonus
- [ ] Morale decay without Ale
- [ ] Morale → combat HP scaling
- [ ] Morale → Scholar XP scaling
- [ ] Low morale negative event pool
- [ ] High morale fatigue reduction
- [ ] Housing / overcrowding morale penalty

---

## Week 4 — Hero Units & Combat Expansion

- [x] Hero dwarf trait system (5% hire chance)
- [x] 6+ hero traits (Ironhide, Berserker, Scholar-King, Foreman…)
- [x] Hero trait display in dwarf panel
- [x] Raid type variety (Goblin, Troll, Necromancer, Dragon)
- [x] Raid type tied to depth + RNG
- [x] Trap upgrade category
- [x] Spike Pit, Flame Vent, Ballista
- [x] Trap passive damage per combat round
- [ ] Trap upkeep

---

## Week 5 — Economy, Trade & Automation

- [x] Caravan system (food cost, 50-tick trip)
- [x] Trade resource → gold / blueprints
- [x] Ambush risk + guard escort option
- [ ] Dynamic trade pricing (surplus = lower price)
- [ ] Foreman upgrade
- [ ] Configurable auto-assignment rules
- [ ] Blueprint presets (up to 4, saved to file)
- [ ] Quick-switch between presets

---

## Week 6 — Endgame & Polish

- [x] World Wonder: Throne of the Mountain King
- [x] World Wonder: World Drill (unlocks Ascension)
- [x] World Wonder: Divine Hammer
- [ ] Ascension biome: Frozen Peak
- [ ] Ascension biome: Volcano Core
- [ ] Ascension biome: Haunted Range
- [ ] Ascension biome: Golden Mountain
- [ ] Event text expanded to 128 entries
- [ ] SDL_mixer sound effect hooks
- [x] In-game help panel (? key)
- [x] High score / achievements screen
- [ ] Final balance pass

---

## Notes

- Bump `SAVE_VERSION` after every `GameState` structural change
- Verify `offsetof()` after every struct change
- New ASM modules: check stack alignment and PIE compliance