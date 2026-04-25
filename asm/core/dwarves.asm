; =========================================================
; asm/core/dwarves.asm - Dwarf morale & fatigue system
;
; No external calls made here. Events are written into
; state->pending and flushed by tick.asm after this returns.
; =========================================================

%include "core/offsets.inc"

%define FATIGUE_MAX     100
%define FATIGUE_WARN    70
%define MORALE_MAX      100

section .data
fatigue_rate:   db 0, 3, 2, 1, 2, 1

section .text
    global asm_tick_dwarves

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

    ; clear pending event
    mov     byte [rbx + GS_PENDING + PENDING_CODE], 0xFF

.loop:
    cmp     r13, MAX_DWARVES
    jge     .done

    movzx   eax, byte [r12 + DWARF_ALIVE]
    test    al, al
    jz      .next_dwarf

    movzx   r14d, byte [r12 + DWARF_JOB]
    movzx   r15d, byte [r12 + DWARF_FATIGUE]

    test    r14d, r14d
    jz      .resting

    ; =======================================================
    ; WORKING: accumulate fatigue
    ; =======================================================
    lea     rax, [rel fatigue_rate]
    movzx   eax, byte [rax + r14]
    add     r15d, eax

    cmp     r15d, FATIGUE_MAX
    jl      .check_warn

    ; -- exhaustion: force idle --
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

    ; write pending event (overwrite previous — last dwarf wins per tick)
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x38
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_NEGATIVE
    mov     [rbx + GS_PENDING + PENDING_DWARF], r13b
    jmp     .store_fatigue

.check_warn:
    cmp     r15d, FATIGUE_WARN
    jl      .store_fatigue
    movzx   eax, byte [r12 + DWARF_MORALE]
    test    eax, eax
    jz      .store_fatigue
    dec     eax
    mov     [r12 + DWARF_MORALE], al
    jmp     .store_fatigue

    ; =======================================================
    ; RESTING: drain fatigue, recover morale
    ; =======================================================
.resting:
    sub     r15d, 2
    jge     .check_return
    xor     r15d, r15d

.check_return:
    test    r15d, r15d
    jnz     .morale_recover

    movzx   eax, byte [r12 + DWARF_PREV_JOB]
    test    al, al
    jz      .morale_recover

    ; return to previous job
    mov     [r12 + DWARF_JOB], al
    mov     byte [r12 + DWARF_PREV_JOB], JOB_IDLE

    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x29
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_POSITIVE
    mov     [rbx + GS_PENDING + PENDING_DWARF], r13b

.morale_recover:
    mov     rax, [rbx + GS_RESOURCES + RES_FOOD]
    test    rax, rax
    jle     .store_fatigue
    movzx   eax, byte [r12 + DWARF_MORALE]
    cmp     eax, MORALE_MAX
    jge     .store_fatigue
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