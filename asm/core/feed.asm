; =========================================================
; asm/core/feed.asm - Feed a dwarf to restore fatigue/morale
;
; asm_feed_dwarf(state, dwarf_idx) -> rax (1=ok, 0=fail)
;   rdi = state
;   rsi = dwarf index
;
; Cost: FEED_FOOD_COST food
; Effect: -FEED_FATIGUE_RESTORE fatigue, +FEED_MORALE_RESTORE morale
; Fails if: insufficient food, dwarf dead, invalid index
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  push rbx: 24  push r12: 32
;   3 pushes, NO calls — alignment irrelevant.
; =========================================================

%include "core/offsets.inc"

%define FEED_FOOD_COST        10
%define FEED_FATIGUE_RESTORE  20
%define FEED_MORALE_RESTORE   10

section .text
    global asm_feed_dwarf

asm_feed_dwarf:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12

    mov     rbx, rdi

    ; bounds check
    cmp     rsi, MAX_DWARVES
    jge     .fail

    ; get dwarf ptr
    imul    rax, rsi, SIZEOF_DWARF
    lea     r12, [rbx + GS_DWARVES + rax]

    ; must be alive
    movzx   eax, byte [r12 + DWARF_ALIVE]
    test    al, al
    jz      .fail

    ; check food
    cmp     qword [rbx + GS_RESOURCES + RES_FOOD], FEED_FOOD_COST
    jl      .fail

    ; deduct food
    sub     qword [rbx + GS_RESOURCES + RES_FOOD], FEED_FOOD_COST

    ; restore fatigue: max(0, fatigue - FEED_FATIGUE_RESTORE)
    movzx   eax, byte [r12 + DWARF_FATIGUE]
    sub     eax, FEED_FATIGUE_RESTORE
    jge     .store_fatigue
    xor     eax, eax
.store_fatigue:
    mov     [r12 + DWARF_FATIGUE], al

    ; restore morale: min(100, morale + FEED_MORALE_RESTORE)
    movzx   eax, byte [r12 + DWARF_MORALE]
    add     eax, FEED_MORALE_RESTORE
    cmp     eax, MORALE_MAX
    jle     .store_morale
    mov     eax, MORALE_MAX
.store_morale:
    mov     [r12 + DWARF_MORALE], al

    ; queue positive event
    movzx   ecx, byte [r12 + DWARF_NAME_IDX]
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x29
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_POSITIVE
    mov     [rbx + GS_PENDING + PENDING_DWARF],         sil

    mov     rax, 1
    jmp     .done

.fail:
    xor     rax, rax

.done:
    pop     r12
    pop     rbx
    pop     rbp
    ret