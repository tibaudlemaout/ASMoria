; =========================================================
; asm/math/rng.asm - xorshift64 pseudo-random number generator
;
; Exports:
;   asm_rng_seed(GameState *state, uint64_t seed)  [rdi, rsi]
;   asm_rng_next(GameState *state)                 [rdi] -> rax
; =========================================================

%include "core/offsets.inc"

section .text
    global asm_rng_seed
    global asm_rng_next

; ---------------------------------------------------------
; asm_rng_seed(GameState *state [rdi], uint64_t seed [rsi])
; Seeds the xorshift64 generator. Seed must be non-zero;
; if caller passes 0 we substitute a safe default.
; ---------------------------------------------------------
asm_rng_seed:
    test    rsi, rsi
    jnz     .store
    mov     rsi, 0xDEADC0DEBEEFDEAD  ; safe non-zero default
.store:
    mov     [rdi + GS_RNG_SEED], rsi
    ret

; ---------------------------------------------------------
; asm_rng_next(GameState *state [rdi]) -> uint64_t [rax]
; Returns next pseudo-random 64-bit value and advances state.
; xorshift64 algorithm: x ^= x << 13; x ^= x >> 7; x ^= x << 17
; ---------------------------------------------------------
asm_rng_next:
    mov     rax, [rdi + GS_RNG_SEED]
    mov     rcx, rax
    shl     rcx, 13
    xor     rax, rcx
    mov     rcx, rax
    shr     rcx, 7
    xor     rax, rcx
    mov     rcx, rax
    shl     rcx, 17
    xor     rax, rcx
    mov     [rdi + GS_RNG_SEED], rax
    ret