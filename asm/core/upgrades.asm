; =========================================================
; asm/core/upgrades.asm - All upgrade categories
;
; IDs 0-2: tools      (gold+stone costs, max 3)
; IDs 3-4: workforce  (gold+stone costs, max 3)
; IDs 5:   watchtower (gold+stone, max 3)
; IDs 6:   rune halls (gold+stone+mana after lv1, max 5)
; IDs 7:   mana well  (gold+stone, max 3)
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  push rbx: 24  push r12: 32
;   push r13: 40  push r14: 48  push r15: 56
;   6 pushes, NO calls.
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

    mov     rbx, rdi
    movzx   r12, sil                    ; upgrade id

    cmp     r12, UPGR_COUNT
    jge     .fail

    ; --- get current level ---
    mov     rax, [rbx + GS_UPGR_TIER1]
    mov     rcx, r12
    shl     rcx, 2
    shr     rax, cl
    and     rax, 0xF
    mov     r13, rax                    ; current level

    ; --- check max level by id ---
    cmp     r12, 3
    jl      .chk_tools
    cmp     r12, 5
    jl      .chk_workforce
    cmp     r12, 6
    je      .chk_runehalls
    cmp     r12, 7
    je      .chk_manawell
    ; id 5 = watchtower
    cmp     r13, UPGR_MAX_WATCHTOWER
    jge     .fail
    jmp     .compute_cost

.chk_tools:
    cmp     r13, UPGR_MAX_TOOLS
    jge     .fail
    jmp     .compute_cost

.chk_workforce:
    cmp     r13, UPGR_MAX_WORKFORCE
    jge     .fail
    jmp     .compute_cost

.chk_runehalls:
    cmp     r13, UPGR_MAX_RUNEHALLS
    jge     .fail
    jmp     .compute_cost

.chk_manawell:
    cmp     r13, UPGR_MAX_MANAWELL
    jge     .fail

.compute_cost:
    mov     r15, r13
    inc     r15                         ; next level

    ; --- select cost base and check/deduct ---
    cmp     r12, 3
    jl      .cost_tools
    cmp     r12, 5
    jl      .cost_workforce
    cmp     r12, 6
    je      .cost_runehalls
    cmp     r12, 7
    je      .cost_manawell
    ; id 5 = watchtower
    jmp     .cost_watchtower

.cost_tools:
    mov     r14, UPGR_COST_GOLD_TOOLS
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_TOOLS
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_TOOLS
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    jmp     .write_level

.cost_workforce:
    mov     r14, UPGR_COST_GOLD_WORK
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_WORK
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_WORK
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    jmp     .write_level

.cost_watchtower:
    mov     r14, UPGR_COST_GOLD_WATCH
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_WATCH
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_WATCH
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    jmp     .write_level

.cost_runehalls:
    mov     r14, UPGR_COST_GOLD_RUNE
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_RUNE
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    ; mana cost from level 2 onwards
    cmp     r15, 2
    jl      .rune_no_mana
    mov     rax, UPGR_COST_MANA_RUNE
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_MANA], rax
    jl      .fail
    mov     rax, UPGR_COST_MANA_RUNE
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_MANA], rax
.rune_no_mana:
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_RUNE
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    jmp     .write_level

.cost_manawell:
    mov     r14, UPGR_COST_GOLD_MANA
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_MANA
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_MANA
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax

.write_level:
    mov     rcx, r12
    shl     rcx, 2
    mov     rax, 0xF
    shl     rax, cl
    not     rax
    and     [rbx + GS_UPGR_TIER1], rax
    mov     rax, r15
    shl     rax, cl
    or      [rbx + GS_UPGR_TIER1], rax

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