; =========================================================
; asm/core/prestige.asm - Clan Honor & Ancestor Tree
;
; Exports:
;   asm_buy_pnode(state, node_id) -> rax (1=ok, 0=fail)
;   asm_do_prestige(state)        -> rax (honor earned, 0=fail)
;
; Node unlock requirements and costs are checked here.
; On prestige: game state is reset, prestige bonuses applied.
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  push rbx: 24  push r12: 32
;   push r13: 40  push r14: 48  push r15: 56
;   sub rsp,8: 64  aligned
; =========================================================

%include "core/offsets.inc"

section .data

; Node costs in honor (indexed by node id 0-23)
pnode_costs:
    db 1, 2, 3              ; YIELD 1-3
    db 1, 2, 1, 2, 2        ; START_GOLD 1-2, START_STONE, FOOD_TICK, WOOD_TICK
    db 1, 2, 3, 1, 2, 1, 1, 1, 2   ; BEARDS branch
    db 1, 2, 1, 2, 2, 3, 2          ; REPUTATION branch

; Node prerequisite bitmasks (what must be unlocked first)
; 0 = no prereq
pnode_prereqs:
    dq 0                                    ; YIELD_1     (root)
    dq (1<<0)                               ; YIELD_2     req YIELD_1
    dq (1<<1)                               ; YIELD_3     req YIELD_2
    dq (1<<0)                               ; START_GOLD1 req YIELD_1
    dq (1<<3)                               ; START_GOLD2 req START_GOLD1
    dq (1<<3)                               ; START_STONE req START_GOLD1
    dq (1<<3)                               ; FOOD_TICK   req START_GOLD1
    dq (1<<6)                               ; WOOD_TICK   req FOOD_TICK
    dq 0                                    ; DWF_2       (root)
    dq (1<<8)                               ; DWF_3       req DWF_2
    dq (1<<9)                               ; DWF_4       req DWF_3
    dq (1<<8)                               ; HIRE_10     req DWF_2
    dq (1<<11)                              ; HIRE_25     req HIRE_10
    dq (1<<11)                              ; START_PICK  req HIRE_10
    dq (1<<13)                              ; START_SAW   req START_PICK
    dq (1<<14)                              ; START_IRR   req START_SAW
    dq (1<<9)                               ; START_BARR  req DWF_3
    dq 0                                    ; RUNE_10     (root)
    dq (1<<17)                              ; RUNE_25     req RUNE_10
    dq (1<<17)                              ; START_MANA  req RUNE_10
    dq (1<<19)                              ; START_RUNE_H req START_MANA
    dq (1<<19)                              ; MANA_TICK_1  req START_MANA
    dq (1<<21)                              ; MANA_TICK_2  req MANA_TICK_1
    dq (1<<20)                              ; START_SCHOLAR req START_RUNE_H

section .text
    global asm_buy_pnode
    global asm_do_prestige
    global asm_can_prestige

; ---------------------------------------------------------
; asm_buy_pnode(rdi=state, rsi=node_id) -> rax
; ---------------------------------------------------------
asm_buy_pnode:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi
    movzx   r12, sil                    ; r12 = node id

    cmp     r12, PNODE_COUNT
    jge     .fail

    lea     r13, [rbx + GS_PRESTIGE]

    ; check already unlocked
    mov     rax, [r13 + PRESTIGE_NODES]
    bt      rax, r12
    jc      .fail                       ; already owned

    ; check prereqs
    lea     rcx, [rel pnode_prereqs]
    mov     r14, [rcx + r12 * 8]       ; r14 = required bitmask
    mov     rax, [r13 + PRESTIGE_NODES]
    and     rax, r14
    cmp     rax, r14                    ; all prereqs met?
    jne     .fail

    ; check honor cost
    lea     rcx, [rel pnode_costs]
    movzx   r15, byte [rcx + r12]      ; r15 = cost
    mov     eax, [r13 + PRESTIGE_HONOR]
    cmp     eax, r15d
    jl      .fail

    ; deduct honor and set node bit
    sub     [r13 + PRESTIGE_HONOR], r15d
    mov     rcx, r12
    mov     rax, 1
    shl     rax, cl
    or      [r13 + PRESTIGE_NODES], rax

    mov     rax, 1
    jmp     .done

.fail:
    xor     rax, rax
.done:
    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

; ---------------------------------------------------------
; asm_can_prestige(rdi=state) -> rax (1=yes, 0=no)
; ---------------------------------------------------------
asm_can_prestige:
    push    rbx
    mov     rbx, rdi

    ; tick >= PRESTIGE_MIN_TICKS
    cmp     qword [rbx + GS_TICK], PRESTIGE_MIN_TICKS
    jl      .no

    ; raids_completed >= PRESTIGE_MIN_RAIDS
    cmp     dword [rbx + GS_RAID + RAID_COMPLETED], PRESTIGE_MIN_RAIDS
    jl      .no

    ; total_resources >= PRESTIGE_MIN_RESOURCES
    cmp     qword [rbx + GS_PRESTIGE + PRESTIGE_RESOURCES], PRESTIGE_MIN_RESOURCES
    jl      .no

    ; count runes inscribed — count non-zero nibbles in tier2
    mov     rax, [rbx + GS_UPGR_TIER2]
    xor     ecx, ecx
    mov     edx, RUNE_COUNT
.nibble_loop:
    test    edx, edx
    jz      .nibble_done
    mov     r8, rax
    and     r8, 0xF
    test    r8, r8
    jz      .nibble_next
    inc     ecx
.nibble_next:
    shr     rax, 4
    dec     edx
    jmp     .nibble_loop
.nibble_done:
    cmp     ecx, PRESTIGE_MIN_RUNES
    jl      .no

    mov     eax, 1
    pop     rbx
    ret
.no:
    xor     eax, eax
    pop     rbx
    ret

; ---------------------------------------------------------
; asm_do_prestige(rdi=state) -> rax (honor earned, 0=fail)
; Resets game state, preserves prestige, applies bonuses.
; ---------------------------------------------------------
asm_do_prestige:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi

    ; check can prestige
    call    asm_can_prestige
    test    eax, eax
    jz      .fail

    ; calculate honor earned
    xor     r12d, r12d                  ; r12 = honor

    ; base: 3
    add     r12d, 3

    ; +1 per raid repelled
    mov     eax, [rbx + GS_RAID + RAID_COMPLETED]
    add     r12d, eax

    ; +1 per rune fully inscribed (max stacks)
    mov     rax, [rbx + GS_UPGR_TIER2]
    xor     ecx, ecx
    mov     edx, RUNE_COUNT
.honor_rune_loop:
    test    edx, edx
    jz      .honor_rune_done
    mov     r8, rax
    and     r8, 0xF
    ; check if maxed (small runes max 5, large max 3)
    ; simplified: any stack >= 3 counts
    cmp     r8, 3
    jl      .honor_rune_next
    inc     ecx
.honor_rune_next:
    shr     rax, 4
    dec     edx
    jmp     .honor_rune_loop
.honor_rune_done:
    add     r12d, ecx

    ; +1 per PRESTIGE_MIN_RESOURCES (10000) total resources
    mov     rax, [rbx + GS_PRESTIGE + PRESTIGE_RESOURCES]
    xor     rdx, rdx
    mov     rcx, PRESTIGE_MIN_RESOURCES
    div     rcx
    add     r12d, eax

    ; save prestige state before reset
    lea     r13, [rbx + GS_PRESTIGE]
    mov     r14d, [r13 + PRESTIGE_HONOR]
    add     r14d, r12d                  ; add new honor
    mov     r15d, [r13 + PRESTIGE_TOTAL_HON]
    add     r15d, r12d
    mov     r9d,  [r13 + PRESTIGE_COUNT]
    inc     r9d
    mov     r10, [r13 + PRESTIGE_NODES]

    ; reset GameState (zero everything)
    ; we need to preserve prestige data — save it on stack
    push    r14                         ; honor
    push    r15                         ; total_honor
    push    r9                          ; count (in r9d, push 64-bit)
    push    r10                         ; nodes

    ; zero the whole state
    mov     rdi, rbx
    mov     rsi, 0
    mov     rdx, GS_SIZE
    ; call memset — but we don't have it linked
    ; manually zero with rep stosq
    xor     rax, rax
    mov     rdi, rbx
    mov     rcx, GS_SIZE / 8
    rep     stosq

    ; restore prestige data
    pop     r10                         ; nodes
    pop     r9                          ; count
    pop     r15                         ; total_honor
    pop     r14                         ; honor

    lea     r13, [rbx + GS_PRESTIGE]
    mov     [r13 + PRESTIGE_HONOR],     r14d
    mov     [r13 + PRESTIGE_TOTAL_HON], r15d
    mov     [r13 + PRESTIGE_COUNT],     r9d
    mov     [r13 + PRESTIGE_NODES],     r10

    ; apply starting bonuses from nodes
    ; Starting resources
    mov     rax, 10                     ; base gold
    bt      r10, PNODE_START_GOLD_1
    jnc     .no_gold1
    add     rax, 50
.no_gold1:
    bt      r10, PNODE_START_GOLD_2
    jnc     .no_gold2
    add     rax, 100
.no_gold2:
    mov     [rbx + GS_RESOURCES + RES_GOLD], rax

    mov     rax, 50                     ; base stone
    bt      r10, PNODE_START_STONE
    jnc     .no_stone
    add     rax, 50
.no_stone:
    mov     [rbx + GS_RESOURCES + RES_STONE], rax

    mov     qword [rbx + GS_RESOURCES + RES_WOOD], 30
    mov     qword [rbx + GS_RESOURCES + RES_FOOD], 20

    ; Starting mana
    xor     rax, rax
    bt      r10, PNODE_START_MANA
    jnc     .no_start_mana
    add     rax, 100
.no_start_mana:
    mov     [rbx + GS_RESOURCES + RES_MANA], rax

    ; Starting upgrades
    xor     eax, eax
    bt      r10, PNODE_START_PICK
    jnc     .no_pick
    or      eax, (1 << (UPGR_PICK_QUALITY * 4))
.no_pick:
    bt      r10, PNODE_START_SAW
    jnc     .no_saw
    or      eax, (1 << (UPGR_SAW_QUALITY * 4))
.no_saw:
    bt      r10, PNODE_START_IRR
    jnc     .no_irr
    or      eax, (1 << (UPGR_IRRIGATION * 4))
.no_irr:
    bt      r10, PNODE_START_BARR
    jnc     .no_barr
    or      eax, (1 << (UPGR_BARRACKS * 4))
.no_barr:
    bt      r10, PNODE_START_RUNE_H
    jnc     .no_rune_h
    or      eax, (1 << (UPGR_RUNE_HALLS * 4))
.no_rune_h:
    mov     [rbx + GS_UPGR_TIER1], rax

    ; Starting dwarves
    mov     ecx, 1                      ; base 1 dwarf
    bt      r10, PNODE_START_DWF_2
    jnc     .dwf_done
    mov     ecx, 2
    bt      r10, PNODE_START_DWF_3
    jnc     .dwf_done
    mov     ecx, 3
    bt      r10, PNODE_START_DWF_4
    jnc     .dwf_done
    mov     ecx, 4
.dwf_done:
    ; spawn dwarves
    lea     rdi, [rbx + GS_DWARVES]
    xor     edx, edx
.spawn_loop:
    cmp     edx, ecx
    jge     .spawn_done
    mov     byte [rdi + DWARF_ALIVE],     1
    mov     byte [rdi + DWARF_JOB],       JOB_MINER
    mov     byte [rdi + DWARF_MORALE],    80
    mov     byte [rdi + DWARF_FATIGUE],   0
    mov     byte [rdi + DWARF_PREV_JOB],  JOB_IDLE
    mov     byte [rdi + DWARF_EQUIPMENT], EQUIP_NONE
    add     rdi, SIZEOF_DWARF
    inc     edx
    jmp     .spawn_loop
.spawn_done:

    ; restore misc state
    mov     byte [rbx + GS_PENDING + PENDING_CODE], 0xFF
    mov     dword [rbx + GS_DEPTH], 1
    mov     qword [rbx + GS_RAID + RAID_NEXT_TICK], 0

    ; RNG seed — use prestige count as entropy
    mov     rax, r9
    mov     rcx, 0xA5710A1A000
    imul    rax, rcx
    mov     rcx, 0xDEADBEEFCAFE
    xor     rax, rcx
    mov     [rbx + GS_RNG_SEED], rax

    mov     rax, r12                    ; return honor earned
    jmp     .done

.fail:
    xor     rax, rax
.done:
    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret