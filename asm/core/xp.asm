; =========================================================
; asm/core/xp.asm - Per-job XP and level system
;
; asm_tick_xp(GameState *state)
;   Called each tick. For each alive dwarf:
;   - If working: add XP to current job's pool
;   - If player-idle: add XP to idle pool
;   - Check if any job crossed a level threshold
;   - On level-up: increment job_level, queue milestone event
;
; XP rates (per tick):
;   Idle (player-set): XP_RATE_IDLE
;   Miner/Lumberer:    XP_RATE_MINER/LUMBERER
;   Farmer/Guard/Scholar: XP_RATE_FARMER etc.
;
; Level thresholds: 0 / 500 / 1500 / 3500 / 7500
;
; Level bonuses applied elsewhere (resources.asm, dwarves.asm)
; by reading DWARF_JOB_LEVEL[job].
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  aligned
;   push rbx: 24
;   push r12: 32  aligned
;   push r13: 40
;   push r14: 48  aligned
;   push r15: 56
;   sub rsp,8: 64  aligned  <- calls to asm_event_push via GS_PENDING
;   6 pushes + sub rsp,8. NO direct calls — writes GS_PENDING.
; =========================================================

%include "core/offsets.inc"

; XP thresholds table (level 1..5 thresholds)
section .data
xp_thresholds: dq 500, 1500, 3500, 7500, 0  ; 0 = max level sentinel

; XP gain rates indexed by job id
xp_rates: db 0, 3, 3, 2, 2, 2  ; idle=0(handled separately), miner=3, ...

section .text
    global asm_tick_xp

asm_tick_xp:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi                    ; rbx = state
    lea     r12, [rbx + GS_DWARVES]    ; r12 = &dwarves[0]
    xor     r13, r13                    ; r13 = dwarf index

.loop:
    cmp     r13, MAX_DWARVES
    jge     .done

    movzx   eax, byte [r12 + DWARF_ALIVE]
    test    al, al
    jz      .next_dwarf

    movzx   r14d, byte [r12 + DWARF_JOB]       ; r14 = current job
    movzx   r15d, byte [r12 + DWARF_PREV_JOB]  ; r15 = prev_job

    ; --- determine which job gets XP this tick ---
    ; force-resting (prev_job != IDLE): no XP
    test    r15d, r15d
    jnz     .next_dwarf

    ; player-idle: idle job gets XP
    test    r14d, r14d
    jz      .idle_xp

    ; working: current job gets XP
    lea     rax, [rel xp_rates]
    movzx   eax, byte [rax + r14]      ; rate for this job
    test    eax, eax
    jz      .next_dwarf

    ; add XP to job_xp[job]
    mov     rcx, r14
    shl     rcx, 3                     ; * 8 (sizeof int64_t)
    add     [r12 + DWARF_JOB_XP + rcx], rax
    jmp     .check_levelup

.idle_xp:
    ; idle job (index 0) gets XP_RATE_IDLE per tick
    add     qword [r12 + DWARF_JOB_XP + 0], XP_RATE_IDLE
    xor     r14d, r14d                 ; job = 0 for level check

.check_levelup:
    ; check if job r14 levelled up
    ; current level = job_level[r14]
    movzx   eax, byte [r12 + DWARF_JOB_LEVEL + r14]
    cmp     eax, MAX_JOB_LEVEL
    jge     .next_dwarf                ; already max level

    ; threshold = xp_thresholds[current_level]
    mov     rcx, rax
    shl     rcx, 3                     ; * 8
    lea     rdx, [rel xp_thresholds]
    mov     rdx, [rdx + rcx]           ; rdx = threshold for next level
    test    rdx, rdx
    jz      .next_dwarf                ; sentinel: max level

    ; current xp = job_xp[r14]
    mov     rcx, r14
    shl     rcx, 3
    mov     rax, [r12 + DWARF_JOB_XP + rcx]

    cmp     rax, rdx
    jl      .next_dwarf                ; not there yet

    ; --- level up! ---
    inc     byte [r12 + DWARF_JOB_LEVEL + r14]

    ; queue milestone event 0x43 "a dwarf levels up"
    ; only if no pending event already
    movzx   eax, byte [rbx + GS_PENDING + PENDING_CODE]
    cmp     al, 0xFF
    jne     .next_dwarf                ; don't overwrite existing pending

    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x43
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_POSITIVE
    mov     [rbx + GS_PENDING + PENDING_DWARF],         r13b

.next_dwarf:
    add     r12, SIZEOF_DWARF
    inc     r13
    jmp     .loop

.done:
    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret