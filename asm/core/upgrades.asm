; =========================================================
; asm/core/upgrades.asm - Tools upgrade system (first pass)
;
; Upgrades 0-2 (PICK_QUALITY, SAW_QUALITY, IRRIGATION)
; packed as 4-bit nibbles in GS_UPGR_TIER1.
;
; asm_buy_upgrade(state, id) -> rax = new level or 0
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16         aligned
;   push rbx: 24
;   push r12: 32         aligned
;   push r13: 40
;   push r14: 48         aligned
;   push r15: 56
;   Total: 6 pushes, NO calls — no sub rsp needed.
; =========================================================

%include "core/offsets.inc"

section .text
    global asm_buy_upgrade

asm_buy_upgrade:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rbx, rdi                    ; rbx = state
    movzx   r12, sil                    ; r12 = upgrade id

    ; bounds check: only 0-2 for now
    cmp     r12, UPGR_COUNT
    jge     .fail

    ; get current level: (tier1 >> (id*4)) & 0xF
    mov     rax, [rbx + GS_UPGR_TIER1]
    mov     rcx, r12
    shl     rcx, 2                      ; rcx = id * 4
    shr     rax, cl
    and     rax, 0xF
    mov     r13, rax                    ; r13 = current level

    ; check max level
    cmp     r13, UPGR_MAX_LEVEL
    jge     .fail

    ; next_level = current + 1
    mov     r15, r13
    inc     r15                         ; r15 = next level

    ; cost = base * next_level
    mov     r14, UPGR_COST_GOLD_BASE
    imul    r14, r15                    ; r14 = gold cost
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail

    mov     rax, UPGR_COST_STONE_BASE
    imul    rax, r15                    ; rax = stone cost
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail

    ; deduct resources
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_BASE
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax

    ; write new level into tier1 nibble
    ; clear:  tier1 &= ~(0xF << (id*4))
    ; set:    tier1 |=  (next_level << (id*4))
    mov     rcx, r12
    shl     rcx, 2
    mov     rax, 0xF
    shl     rax, cl
    not     rax
    and     [rbx + GS_UPGR_TIER1], rax
    mov     rax, r15
    shl     rax, cl
    or      [rbx + GS_UPGR_TIER1], rax

    ; queue milestone event 0x41
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x41
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_MILESTONE
    mov     byte [rbx + GS_PENDING + PENDING_DWARF],    0xFF

    mov     rax, r15
    jmp     .done

.fail:
    xor     rax, rax

.done:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret