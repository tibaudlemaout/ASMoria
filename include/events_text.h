#ifndef EVENTS_TEXT_H
#define EVENTS_TEXT_H

/* =========================================================
 * events_text.h
 * String tables for event codes produced by events.asm.
 *
 * The ASM engine writes a numeric code; the UI looks up the
 * human-readable string here. Dwarf name substitution uses
 * the dwarf_idx field — for now dwarves are named by index
 * ("Dwarf #3") until a name system is added.
 *
 * Code ranges:
 *   0x00-0x1F  flavour  (32 entries) - miners at work
 *   0x20-0x2F  positive (16 entries) - good stat events
 *   0x30-0x3F  negative (16 entries) - bad stat events
 *   0x40-0x4F  milestone(16 entries) - depth/upgrades
 * ========================================================= */

/* %s is replaced with the dwarf's name/label at render time */

static const char *evt_flavour[32] = {
    /* 0x00 */ "%s strikes a promising vein of stone.",
    /* 0x01 */ "%s hums an old tune while swinging the pick.",
    /* 0x02 */ "Dust rises as %s breaks through a new passage.",
    /* 0x03 */ "%s pauses to sharpen their pickaxe.",
    /* 0x04 */ "The sound of %s's work echoes through the hall.",
    /* 0x05 */ "%s uncovers a cluster of strange crystals.",
    /* 0x06 */ "%s grumbles but keeps on digging.",
    /* 0x07 */ "A distant rumble — %s doesn't flinch.",
    /* 0x08 */ "%s finds a fossilised bone and pockets it.",
    /* 0x09 */ "%s whistles a march from the old wars.",
    /* 0x0A */ "Sparks fly as %s's pick strikes flint.",
    /* 0x0B */ "%s takes a long swig of ale before resuming.",
    /* 0x0C */ "The torch flickers. %s digs on regardless.",
    /* 0x0D */ "%s carved a small rune into the tunnel wall.",
    /* 0x0E */ "%s found a smooth river stone deep underground.",
    /* 0x0F */ "A cave beetle scurries past %s's boots.",
    /* 0x10 */ "%s argues with the foreman, then gets back to work.",
    /* 0x11 */ "%s notices the tunnel is getting warmer.",
    /* 0x12 */ "Water seeps through the wall near %s.",
    /* 0x13 */ "%s spots something glinting in the rock face.",
    /* 0x14 */ "The ceiling groans. %s eyes it warily.",
    /* 0x15 */ "%s measures the tunnel with a knotted rope.",
    /* 0x16 */ "%s's lantern needs trimming but they press on.",
    /* 0x17 */ "A stalactite drips steadily near %s's station.",
    /* 0x18 */ "%s stops to sketch the rock strata.",
    /* 0x19 */ "Deep below, %s hears the mountain breathe.",
    /* 0x1A */ "%s finds a pocket of cold air in the wall.",
    /* 0x1B */ "%s sets a support beam with practiced hands.",
    /* 0x1C */ "A distant drip marks the hours for %s.",
    /* 0x1D */ "%s chips a perfect cube from the rock face.",
    /* 0x1E */ "The smell of iron ore reaches %s's nose.",
    /* 0x1F */ "%s catches a glimpse of something vast beyond the wall.",
};

static const char *evt_positive[16] = {
    /* 0x20 */ "%s finds a rich gold seam! [+MORALE]",
    /* 0x21 */ "%s's hard work pays off — a bonus haul! [+XP]",
    /* 0x22 */ "%s discovers a hidden cache of provisions. [+FOOD]",
    /* 0x23 */ "%s's technique improves with every swing. [+XP]",
    /* 0x24 */ "%s unearths a mana crystal! [+MANA]",
    /* 0x25 */ "The foreman praises %s's output. [+MORALE]",
    /* 0x26 */ "%s breaks through to an untouched gallery. [+STONE]",
    /* 0x27 */ "%s finds a natural spring. [+FOOD, +MORALE]",
    /* 0x28 */ "%s's experienced eye spots a structural weakness. [+XP]",
    /* 0x29 */ "%s rests well and returns refreshed. [-FATIGUE]",
    /* 0x2A */ "%s trains a younger dwarf in passing. [+XP]",
    /* 0x2B */ "%s finds an ancient tool — still sharp. [+MORALE]",
    /* 0x2C */ "%s strikes a vein so pure it gleams gold. [+GOLD]",
    /* 0x2D */ "A building has been restored — upkeep resumed.",
    /* 0x2E */ "%s solves a long-standing drainage problem. [+XP]",
    /* 0x2F */ "%s uncovers a map scratched into the stone. [+MORALE]",
};

static const char *evt_negative[16] = {
    /* 0x30 */ "%s twists an ankle on loose rubble. [+FATIGUE]",
    /* 0x31 */ "%s breathes too much dust. Coughing badly. [+FATIGUE]",
    /* 0x32 */ "%s's pickaxe handle splinters. Output reduced. [-MORALE]",
    /* 0x33 */ "A small cave-in buries %s's work for the day. [-MORALE]",
    /* 0x34 */ "%s argues with a colleague and sulks. [-MORALE]",
    /* 0x35 */ "%s skips their meal. Fatigue rises. [+FATIGUE]",
    /* 0x36 */ "%s is spooked by strange noises in the deep. [-MORALE]",
    /* 0x37 */ "The foreman reprimands %s for sloppy shoring. [-MORALE]",
    /* 0x38 */ "%s overexerts trying to prove themselves. [+FATIGUE]",
    /* 0x39 */ "%s finds their tools have been pilfered. [-MORALE]",
    /* 0x3A */ "Foul air in the tunnel makes %s dizzy. [+FATIGUE]",
    /* 0x3B */ "%s strikes rock so hard the pick bounces back. [+FATIGUE]",
    /* 0x3C */ "A drip of scalding water burns %s's arm. [+FATIGUE]",
    /* 0x3D */ "%s receives bad news from the surface. [-MORALE]",
    /* 0x3E */ "%s's lantern goes out. An hour lost to darkness. [-MORALE]",
    /* 0x3F */ "%s sees something in the dark. Won't speak of it. [-MORALE]",
};

static const char *evt_milestone[16] = {
    /* 0x40 */ "The halls grow deeper. Depth increased!",
    /* 0x41 */ "A new upgrade has been unlocked.",
    /* 0x42 */ "The first dwarf joins the workforce.",
    /* 0x43 */ "%s has mastered a new level of their craft!",
    /* 0x44 */ "A grand hall has been carved out.",
    /* 0x45 */ "The treasury overflows with gold.",
    /* 0x46 */ "The dwarves reach a new stratum of rock.",
    /* 0x47 */ "An ancient door is found, sealed with runes.",
    /* 0x48 */ "The mountain itself seems to watch.",
    /* 0x49 */ "A great vein of mithril is rumoured below.",
    /* 0x4A */ "The chronicles are damaged. A new story begins from ruin.",
    /* 0x4B */ "The chronicles of ASMoria have been preserved in stone.",
    /* 0x4C */ "The first scholar joins — knowledge accumulates.",
    /* 0x4D */ "Guards are posted. Something stirs below.",
    /* 0x4E */ "ASMoria grows. Word spreads to distant holds.",
    /* 0x4F */ "The deep calls. Who will answer?",
    /* 0x50 */ "The clan digs deeper. The mountain yields its secrets.",
    /* 0x51 */ "You have reached the limits of your knowledge. Research the Rune of the Deep.",
};

/* Look up a message string by raw event code.
   Returns NULL if code is out of range. */
static inline const char *evt_get_template(uint8_t code) {
    if (code <= 0x1F) return evt_flavour[code & 0x1F];
    if (code <= 0x2F) return evt_positive[code & 0x0F];
    if (code <= 0x3F) return evt_negative[code & 0x0F];
    if (code <= 0x4F) return evt_milestone[code & 0x0F];
    if (code <= 0x5F) return evt_milestone[0x10 + (code & 0x0F)];
    return "Something stirs in the dark.";
}

#endif /* EVENTS_TEXT_H */