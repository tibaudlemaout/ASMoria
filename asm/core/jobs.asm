; =========================================================
; asm/core/jobs.asm - Dwarf job assignment
;
; asm_assign_job(state, dwarf_idx, job)
;   rdi = state, rsi = dwarf index, rdx = new job
;   rax = 1 success, 0 failure
;
; Guard requires Watch Tower >= 1
; Scholar requires Rune Halls >= 1
;
; STACK: no pushes, no calls.
; =========================================================

%include "core/offsets.inc"

section .text
    global asm_assign_job

asm_assign_job:
    cmp     rsi, MAX_DWARVES
    jge     .fail

    imul    rax, rsi, SIZEOF_DWARF
    lea     rcx, [rdi + GS_DWARVES + rax]

    ; must be alive
    movzx   eax, byte [rcx + DWARF_ALIVE]
    test    al, al
    jz      .fail

    ; refuse if force-resting
    movzx   eax, byte [rcx + DWARF_PREV_JOB]
    test    al, al
    jnz     .fail

    ; check job unlock requirements
    cmp     dl, JOB_GUARD
    je      .check_guard
    cmp     dl, JOB_SCHOLAR
    je      .check_scholar
    jmp     .assign

.check_guard:
    mov     rax, [rdi + GS_UPGR_TIER1]
    shr     rax, (UPGR_WATCH_TOWER * 4)
    and     rax, 0xF
    test    rax, rax
    jz      .fail               ; Watch Tower not built
    jmp     .assign

.check_scholar:
    mov     rax, [rdi + GS_UPGR_TIER1]
    shr     rax, (UPGR_RUNE_HALLS * 4)
    and     rax, 0xF
    test    rax, rax
    jz      .fail               ; Rune Halls not built
    jmp     .assign

.assign:
    mov     [rcx + DWARF_JOB],           dl
    mov     byte [rcx + DWARF_PREV_JOB], JOB_IDLE
    mov     rax, 1
    ret

.fail:
    xor     rax, rax
    ret