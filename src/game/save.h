#ifndef SAVE_H
#define SAVE_H

#include "../../include/asmoria.h"
#include <stdint.h>

/* =========================================================
 * ASMoria save system
 *
 * Save file layout:
 *   [0..3]   magic    : 0x41534D52  ("ASMR")
 *   [4..7]   version  : SAVE_VERSION
 *   [8..15]  gs_size  : sizeof(GameState)
 *   [16..N]  payload  : raw GameState bytes
 *   [N..N+7] checksum : FNV-1a checksum of payload (uint64)
 *
 * The header and checksum are validated before loading.
 * All file I/O is done via raw Linux syscalls in save.asm.
 * ========================================================= */

#define SAVE_MAGIC          0x41534D52u
#define SAVE_VERSION        3u
#define SAVE_PATH           "asmoria.sav"
#define AUTOSAVE_INTERVAL   300

/* Return codes from asm_save_game / asm_load_game */
#define SAVE_OK             0
#define SAVE_ERR_OPEN       1
#define SAVE_ERR_WRITE      2
#define SAVE_ERR_READ       3
#define SAVE_ERR_MAGIC      4
#define SAVE_ERR_VERSION    5
#define SAVE_ERR_SIZE       6
#define SAVE_ERR_CHECKSUM   7
#define SAVE_ERR_NOFILE     8

typedef int SaveResult;

/* Save header written/read by ASM */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint64_t gs_size;
} SaveHeader;              /* size: 16 bytes */

/* =========================================================
 * ASM-exported functions
 * ========================================================= */

/* Compute FNV-1a checksum over [data .. data+len) -> rax */
extern uint64_t asm_state_checksum(const void *data, uint64_t len);

/* Save GameState to path.
 * path     : null-terminated file path
 * state    : pointer to GameState
 * Returns SAVE_* code.
 */
extern int64_t asm_save_game(const char *path, const GameState *state);

/* Load GameState from path.
 * path     : null-terminated file path
 * state    : pointer to GameState (written on success)
 * Returns SAVE_* code.
 */
extern int64_t asm_load_game(const char *path, GameState *state);

/* =========================================================
 * C wrappers
 * ========================================================= */
SaveResult  save_game(const GameState *state);
SaveResult  load_game(GameState *state);
const char *save_result_str(SaveResult r);

#endif /* SAVE_H */