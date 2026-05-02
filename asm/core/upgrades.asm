; =========================================================
; asm/core/upgrades.asm - Tools + Workforce upgrades
;
; asm_buy_upgrade(state, id) -> rax = new level or 0
;
; IDs 0-2: tools     (cost: UPGR_COST_GOLD/STONE_TOOLS * next)
; IDs 3-4: workforce (cost: UPGR_COST_GOLD/STONE_WORK  * next)
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  aligned
;   push rbx: 24
;   push r12: 32  aligned
;   push r13: 40
;   push r14: 48  aligned
;   push r15: 56
;   6 pushes, NO calls — alignment irrelevant.
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

    mov     rbx, rdi                    ; state
    movzx   r12, sil                    ; upgrade id

    cmp     r12, UPGR_COUNT
    jge     .fail

    ; --- get current level ---
    mov     rax, [rbx + GS_UPGR_TIER1]
    mov     rcx, r12
    shl     rcx, 2
    shr     rax, cl
    and     rax, 0xF
    mov     r13, rax                    ; r13 = current level

    ; --- check max level (tools=3, workforce=3) ---
    cmp     r12, 3
    jl      .check_max_tools
    cmp     r13, UPGR_MAX_WORKFORCE
    jge     .fail
    jmp     .compute_cost

.check_max_tools:
    cmp     r13, UPGR_MAX_TOOLS
    jge     .fail

.compute_cost:
    mov     r15, r13
    inc     r15                         ; r15 = next level

    ; --- select cost base by category ---
    cmp     r12, 3
    jl      .tools_cost

    ; workforce cost
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

.tools_cost:
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

.write_level:
    ; clear old nibble, write new level
    mov     rcx, r12
    shl     rcx, 2
    mov     rax, 0xF
    shl     rax, cl
    not     rax
    and     [rbx + GS_UPGR_TIER1], rax
    mov     rax, r15
    shl     rax, cl
    or      [rbx + GS_UPGR_TIER1], rax

    ; queue milestone event
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