; =========================================================
; asm/core/dwarves.asm - Dwarf morale & fatigue system
;
; Morale rules:
;   - Working, fatigue < FATIGUE_WARN : no change
;   - Working, fatigue >= FATIGUE_WARN: -1/tick
;   - Exhaustion (forced idle)        : -5 one-time, no recovery
;   - Player-set idle                 : +1 every MORALE_IDLE_RATE ticks
;
; Fatigue rules:
;   - Working : +rate/tick (by job)
;   - Any idle: -2/tick (both forced and player-set)
;   - Force-rest ends when fatigue == 0, returns to prev_job
;
; No external calls — events written to GS_PENDING.
; =========================================================

%include "core/offsets.inc"

%define FATIGUE_MAX         100
%define FATIGUE_WARN        85
%define MORALE_MAX          100
%define MORALE_IDLE_RATE    10  ; +1 morale every N ticks when player-idle

section .data
fatigue_rate:   db 0, 3, 2, 1, 2, 1

section .text
    global asm_tick_dwarves

; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  aligned
;   push rbx: 24
;   push r12: 32  aligned
;   push r13: 40
;   push r14: 48  aligned
;   push r15: 56
;   6 pushes, NO calls — alignment irrelevant.
asm_tick_dwarves:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rbx, rdi
    lea     r12, [rbx + GS_DWARVES]
    xor     r13, r13

    mov     byte [rbx + GS_PENDING + PENDING_CODE], 0xFF

.loop:
    cmp     r13, MAX_DWARVES
    jge     .done

    movzx   eax, byte [r12 + DWARF_ALIVE]
    test    al, al
    jz      .next_dwarf

    movzx   r14d, byte [r12 + DWARF_JOB]       ; r14 = job
    movzx   r15d, byte [r12 + DWARF_FATIGUE]   ; r15 = fatigue

    test    r14d, r14d
    jz      .idle                               ; JOB_IDLE

    ; =======================================================
    ; WORKING
    ; =======================================================
    lea     rax, [rel fatigue_rate]
    movzx   eax, byte [rax + r14]
    add     r15d, eax

    cmp     r15d, FATIGUE_MAX
    jl      .check_warn

    ; -- exhaustion --
    mov     r15d, FATIGUE_MAX
    mov     al, r14b
    mov     [r12 + DWARF_PREV_JOB], al
    mov     byte [r12 + DWARF_JOB], JOB_IDLE

    movzx   eax, byte [r12 + DWARF_MORALE]
    sub     eax, 5
    jge     .clamp_ex
    xor     eax, eax
.clamp_ex:
    mov     [r12 + DWARF_MORALE], al

    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x38
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_NEGATIVE
    mov     [rbx + GS_PENDING + PENDING_DWARF], r13b
    jmp     .store_fatigue

.check_warn:
    ; drain morale only when very tired
    cmp     r15d, FATIGUE_WARN
    jl      .store_fatigue
    movzx   eax, byte [r12 + DWARF_MORALE]
    test    eax, eax
    jz      .store_fatigue
    dec     eax
    mov     [r12 + DWARF_MORALE], al
    jmp     .store_fatigue

    ; =======================================================
    ; IDLE (player-set or force-resting)
    ; =======================================================
.idle:
    ; drain fatigue — base 2 + idle_level bonus
    movzx   eax, byte [r12 + DWARF_JOB_LEVEL + JOB_IDLE]
    add     eax, 2
    sub     r15d, eax
    jge     .check_return
    xor     r15d, r15d

.check_return:
    ; if force-resting (prev_job set), check if done
    movzx   eax, byte [r12 + DWARF_PREV_JOB]
    test    al, al
    jz      .player_idle            ; prev_job == 0 -> player-set idle

    ; force-resting: no morale recovery
    test    r15d, r15d
    jnz     .store_fatigue          ; still resting, no recovery

    ; fatigue == 0: return to work
    mov     [r12 + DWARF_JOB], al
    mov     byte [r12 + DWARF_PREV_JOB], JOB_IDLE

    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x29
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_POSITIVE
    mov     [rbx + GS_PENDING + PENDING_DWARF], r13b
    jmp     .store_fatigue

.player_idle:
    ; player chose to idle this dwarf — recover morale slowly
    movzx   eax, byte [r12 + DWARF_MORALE]
    cmp     eax, MORALE_MAX
    jge     .store_fatigue

    ; +1 every MORALE_IDLE_RATE ticks
    mov     rax, [rbx + GS_TICK]
    xor     rdx, rdx
    mov     rcx, MORALE_IDLE_RATE
    div     rcx
    test    rdx, rdx
    jnz     .store_fatigue

    movzx   eax, byte [r12 + DWARF_MORALE]
    inc     eax
    mov     [r12 + DWARF_MORALE], al

.store_fatigue:
    mov     [r12 + DWARF_FATIGUE], r15b

.next_dwarf:
    add     r12, SIZEOF_DWARF
    inc     r13
    jmp     .loop

.done:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret