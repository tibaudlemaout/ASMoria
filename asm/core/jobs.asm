; =========================================================
; asm/core/jobs.asm - Dwarf job assignment
;
; asm_assign_job(GameState *state, uint8_t dwarf_idx, uint8_t job)
;   rdi = state
;   rsi = dwarf index (0-63)
;   rdx = new job (JOB_*)
;
; Only assigns if the dwarf is alive and not force-resting.
; Returns 1 on success, 0 on failure (in rax).
; =========================================================

%include "core/offsets.inc"

section .text
    global asm_assign_job

asm_assign_job:
    ; bounds check
    cmp     rsi, MAX_DWARVES
    jge     .fail

    ; get dwarf ptr
    imul    rax, rsi, SIZEOF_DWARF
    lea     rcx, [rdi + GS_DWARVES + rax]

    ; must be alive
    movzx   eax, byte [rcx + DWARF_ALIVE]
    test    al, al
    jz      .fail

    ; if force-resting (prev_job != IDLE), don't override rest
    movzx   eax, byte [rcx + DWARF_PREV_JOB]
    test    al, al
    jnz     .fail

    ; assign job and clear prev_job
    mov     [rcx + DWARF_JOB],      dl
    mov     byte [rcx + DWARF_PREV_JOB], JOB_IDLE

    mov     rax, 1
    ret

.fail:
    xor     rax, rax
    ret