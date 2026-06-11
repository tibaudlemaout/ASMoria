; =========================================================
; asm/core/depth.asm - Mining depth progression
;
; Exports:
;   asm_dig_deeper(state)     -> rax (1=ok, 0=fail/maxed)
;   asm_tick_depth(state)     -> void (per-tick depth yields)
;
; Depth 1: base (no new resources)
; Depth 2: +iron_ore per miner (1 + pick_level)
; Depth 3: +gems per miner (1 per 5 ticks)
; Depth 4: +relics (1 per 20 ticks, random)
; Depth 5: +crystals (1 per 10 ticks)
;
; Yield multiplier per depth (applied in resources.asm):
;   depth 1 = 100%, 2 = 120%, 3 = 140%, 4 = 160%, 5 = 180%
;
; Fatigue penalty per depth (per tick, applied in dwarves.asm):
;   depth 1 = 0, 2 = +1 miners, 3 = +1 all workers,
;   4 = +2 all workers, 5 = +2 all workers + events
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  push rbx: 24  push r12: 32
;   push r13: 40  push r14: 48  push r15: 56
;   sub rsp,8: 64  aligned
; =========================================================

%include "core/offsets.inc"

section .data

; Unlock costs: [stone, gold] per depth level (index = target depth - 2)
depth_cost_stone: dq 500, 1500, 3000, 6000
depth_cost_gold:  dq 300, 1000, 2500, 5000

; Yield multiplier per depth (x100): 100, 120, 140, 160, 180
depth_yield_mult: db 100, 120, 140, 160, 180

; Event codes
%define EVT_DIG_DEEPER      0x50    ; "The clan digs deeper into the mountain."
%define EVT_DEPTH_MAX       0x51    ; "You have reached the limits of your knowledge."

section .text
    global asm_dig_deeper
    global asm_tick_depth
    global asm_get_depth_yield_mult

; ---------------------------------------------------------
; asm_dig_deeper(rdi=state) -> rax (1=ok, 0=fail)
; ---------------------------------------------------------
asm_dig_deeper:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi

    ; get current depth
    mov     r12d, [rbx + GS_DEPTH]

    ; already at max?
    cmp     r12d, DEPTH_MAX
    jge     .fail

    ; check Rune of the Deep gate
    ; max depth = rune_deep_stacks + 1 (base depth 1, +1 per stack up to 4)
    ; so to dig to depth 2 need 1 stack, depth 3 need 2, etc.
    mov     rax, [rbx + GS_UPGR_TIER2]
    shr     rax, (RUNE_DEEP * 4)
    and     rax, 0xF                    ; deep rune stacks
    inc     rax                         ; max reachable depth = stacks + 1
    cmp     r12, rax                    ; current depth >= max allowed?
    jge     .fail

    ; get cost for next depth (index = current depth - 1, since target = current+1)
    ; current depth 1 -> index 0, depth 2 -> index 1 etc.
    mov     r13, r12
    dec     r13                         ; r13 = array index

    lea     rax, [rel depth_cost_stone]
    mov     r14, [rax + r13 * 8]        ; r14 = stone cost

    lea     rax, [rel depth_cost_gold]
    mov     r15, [rax + r13 * 8]        ; r15 = gold cost

    ; check resources
    cmp     [rbx + GS_RESOURCES + RES_STONE], r14
    jl      .fail
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r15
    jl      .fail

    ; deduct and increment depth
    sub     [rbx + GS_RESOURCES + RES_STONE], r14
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r15
    inc     dword [rbx + GS_DEPTH]

    ; queue event
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     EVT_DIG_DEEPER
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_MILESTONE
    mov     byte [rbx + GS_PENDING + PENDING_DWARF],    0xFF

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
; asm_tick_depth(rdi=state) — per-tick depth resource yields
; Called after resources.asm each tick
; ---------------------------------------------------------
asm_tick_depth:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13

    mov     rbx, rdi
    mov     r12d, [rbx + GS_DEPTH]
    mov     r13, [rbx + GS_TICK]

    ; depth 1 = no bonus resources
    cmp     r12d, 1
    je      .done

    ; -------------------------------------------------------
    ; Depth 2+: Iron Ore — 1 per alive miner per tick
    ; (pick upgrade adds to this like stone/gold)
    ; -------------------------------------------------------
    cmp     r12d, 2
    jl      .done

    lea     rax, [rbx + GS_DWARVES]
    mov     rcx, MAX_DWARVES
.iron_loop:
    test    rcx, rcx
    jz      .iron_done
    movzx   edx, byte [rax + DWARF_ALIVE]
    test    dl, dl
    jz      .iron_next
    movzx   edx, byte [rax + DWARF_JOB]
    cmp     dl, JOB_MINER
    jne     .iron_next
    ; iron ore = 1 + pick_level
    mov     r8, [rbx + GS_UPGR_TIER1]
    shr     r8, (UPGR_PICK_QUALITY * 4)
    and     r8, 0xF
    inc     r8
    add     [rbx + GS_RESOURCES + RES_IRON_ORE], r8
.iron_next:
    add     rax, SIZEOF_DWARF
    dec     rcx
    jmp     .iron_loop
.iron_done:

    ; -------------------------------------------------------
    ; Depth 3+: Gems — 1 every 5 ticks (per alive miner)
    ; -------------------------------------------------------
    cmp     r12d, 3
    jl      .done

    mov     rax, r13
    xor     rdx, rdx
    mov     rcx, 5
    div     rcx
    test    rdx, rdx
    jnz     .no_gems

    lea     rax, [rbx + GS_DWARVES]
    mov     rcx, MAX_DWARVES
.gem_loop:
    test    rcx, rcx
    jz      .no_gems
    movzx   edx, byte [rax + DWARF_ALIVE]
    test    dl, dl
    jz      .gem_next
    movzx   edx, byte [rax + DWARF_JOB]
    cmp     dl, JOB_MINER
    jne     .gem_next
    add     qword [rbx + GS_RESOURCES + RES_GEMS], 1
.gem_next:
    add     rax, SIZEOF_DWARF
    dec     rcx
    jmp     .gem_loop
.no_gems:

    ; -------------------------------------------------------
    ; Depth 4+: Relics — 1 every 20 ticks (all miners)
    ; -------------------------------------------------------
    cmp     r12d, 4
    jl      .done

    mov     rax, r13
    xor     rdx, rdx
    mov     rcx, 20
    div     rcx
    test    rdx, rdx
    jnz     .no_relics
    add     qword [rbx + GS_RESOURCES + RES_RELICS], 1
.no_relics:

    ; -------------------------------------------------------
    ; Depth 5: Crystals — 1 every 10 ticks
    ; -------------------------------------------------------
    cmp     r12d, 5
    jl      .done

    mov     rax, r13
    xor     rdx, rdx
    mov     rcx, 10
    div     rcx
    test    rdx, rdx
    jnz     .done
    add     qword [rbx + GS_RESOURCES + RES_CRYSTALS], 1

.done:
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

; ---------------------------------------------------------
; asm_get_depth_yield_mult(rdi=state) -> rax (multiplier x100)
; Used by resources.asm to apply depth yield bonus
; ---------------------------------------------------------
asm_get_depth_yield_mult:
    mov     eax, [rdi + GS_DEPTH]
    cmp     eax, 1
    jl      .base
    cmp     eax, DEPTH_MAX
    jle     .ok
    mov     eax, DEPTH_MAX
.ok:
    dec     eax                         ; 0-based index
    lea     rcx, [rel depth_yield_mult]
    movzx   eax, byte [rcx + rax]
    ret
.base:
    mov     eax, 100
    ret
