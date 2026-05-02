#include "save.h"
#include <stdio.h>

/* =========================================================
 * C wrappers — path string lives here, ASM does the I/O
 * ========================================================= */

SaveResult save_game(const GameState *state) {
    return (SaveResult)asm_save_game(SAVE_PATH, state);
}

SaveResult load_game(GameState *state) {
    return (SaveResult)asm_load_game(SAVE_PATH, state);
}

const char *save_result_str(SaveResult r) {
    switch (r) {
        case SAVE_OK:           return "Save OK";
        case SAVE_ERR_OPEN:     return "Could not open save file";
        case SAVE_ERR_WRITE:    return "Write error";
        case SAVE_ERR_READ:     return "Read error";
        case SAVE_ERR_MAGIC:    return "Not a valid ASMoria save";
        case SAVE_ERR_VERSION:  return "Save version mismatch";
        case SAVE_ERR_SIZE:     return "Save structure mismatch";
        case SAVE_ERR_CHECKSUM: return "Save file is corrupt";
        case SAVE_ERR_NOFILE:   return "No save file found";
        default:                return "Unknown error";
    }
}