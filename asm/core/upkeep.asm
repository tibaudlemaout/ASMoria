; =========================================================
; asm/core/upkeep.asm - Building upkeep per tick
;
; Called once per tick by tick.asm BEFORE infra effects,
; so degraded buildings don't produce before paying.
;
; Upkeep costs (per tick):
;   Watch Tower : level * 1 wood
;   Rune Halls  : level * 1 mana
;   Mana Well   : level * 1 stone
;
; Flags (GS_FLAGS bits):
;   bit 0 = Watch Tower degraded
;   bit 1 = Rune Halls degraded
;   bit 2 = Mana Well degraded
;
; On state change (ok->degraded or degraded->ok):
;   writes a pending event to GS_PENDING.
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  aligned
;   push rbx: 24
;   2 pushes, NO calls — no sub rsp needed.
; =========================================================

%include "core/offsets.inc"

%define FLAG_WATCH_DEGRADED  (1 << 0)
%define FLAG_RUNE_DEGRADED   (1 << 1)
%define FLAG_MANA_DEGRADED   (1 << 2)

; Event codes for degradation changes
%define EVT_DEGRADED         0x3E    ; negative — building degrades
%define EVT_RECOVERED        0x2D    ; positive — building recovers

section .text
    global asm_tick_upkeep

; ---------------------------------------------------------
; asm_tick_upkeep(rdi=state)
; ---------------------------------------------------------
asm_tick_upkeep:
    push    rbp
    mov     rbp, rsp
    push    rbx

    mov     rbx, rdi
    mov     r8,  [rbx + GS_UPGR_TIER1]
    mov     r9d, [rbx + GS_FLAGS]       ; r9 = current flags

    ; -------------------------------------------------------
    ; Watch Tower upkeep: level * 2 wood/tick
    ; -------------------------------------------------------
    mov     rax, r8
    shr     rax, (UPGR_WATCH_TOWER * 4)
    and     rax, 0xF
    test    rax, rax
    jz      .wt_skip

    ; cost = level wood
    shl     rax, 1                      ; * 2
    cmp     [rbx + GS_RESOURCES + RES_WOOD], rax
    jl      .wt_degrade

    ; can pay — deduct and clear degraded flag
    sub     [rbx + GS_RESOURCES + RES_WOOD], rax
    test    r9d, FLAG_WATCH_DEGRADED
    jz      .wt_skip                    ; was already ok
    ; recovered!
    and     r9d, ~FLAG_WATCH_DEGRADED
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     EVT_RECOVERED
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_POSITIVE
    mov     byte [rbx + GS_PENDING + PENDING_DWARF],    0xFF
    jmp     .wt_skip

.wt_degrade:
    test    r9d, FLAG_WATCH_DEGRADED
    jnz     .wt_skip                    ; already degraded, no repeat event
    or      r9d, FLAG_WATCH_DEGRADED
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     EVT_DEGRADED
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_NEGATIVE
    mov     byte [rbx + GS_PENDING + PENDING_DWARF],    0xFF

.wt_skip:
    ; -------------------------------------------------------
    ; Rune Halls upkeep: level * 2 mana/tick
    ; -------------------------------------------------------
    mov     rax, r8
    shr     rax, (UPGR_RUNE_HALLS * 4)
    and     rax, 0xF
    test    rax, rax
    jz      .rh_skip

    shl     rax, 1                      ; * 2
    cmp     [rbx + GS_RESOURCES + RES_MANA], rax
    jl      .rh_degrade

    sub     [rbx + GS_RESOURCES + RES_MANA], rax
    test    r9d, FLAG_RUNE_DEGRADED
    jz      .rh_skip
    ; recovered
    and     r9d, ~FLAG_RUNE_DEGRADED
    ; only push event if no pending already
    movzx   eax, byte [rbx + GS_PENDING + PENDING_CODE]
    cmp     al, 0xFF
    jne     .rh_skip
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     EVT_RECOVERED
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_POSITIVE
    mov     byte [rbx + GS_PENDING + PENDING_DWARF],    0xFF
    jmp     .rh_skip

.rh_degrade:
    test    r9d, FLAG_RUNE_DEGRADED
    jnz     .rh_skip
    or      r9d, FLAG_RUNE_DEGRADED
    movzx   eax, byte [rbx + GS_PENDING + PENDING_CODE]
    cmp     al, 0xFF
    jne     .rh_skip
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     EVT_DEGRADED
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_NEGATIVE
    mov     byte [rbx + GS_PENDING + PENDING_DWARF],    0xFF

.rh_skip:
    ; -------------------------------------------------------
    ; Mana Well upkeep: level * 3 stone/tick
    ; -------------------------------------------------------
    mov     rax, r8
    shr     rax, (UPGR_MANA_WELL * 4)
    and     rax, 0xF
    test    rax, rax
    jz      .mw_skip

    imul    rax, 3                      ; * 3
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .mw_degrade

    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    test    r9d, FLAG_MANA_DEGRADED
    jz      .mw_skip
    and     r9d, ~FLAG_MANA_DEGRADED
    movzx   eax, byte [rbx + GS_PENDING + PENDING_CODE]
    cmp     al, 0xFF
    jne     .mw_skip
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     EVT_RECOVERED
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_POSITIVE
    mov     byte [rbx + GS_PENDING + PENDING_DWARF],    0xFF
    jmp     .mw_skip

.mw_degrade:
    test    r9d, FLAG_MANA_DEGRADED
    jnz     .mw_skip
    or      r9d, FLAG_MANA_DEGRADED
    movzx   eax, byte [rbx + GS_PENDING + PENDING_CODE]
    cmp     al, 0xFF
    jne     .mw_skip
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     EVT_DEGRADED
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_NEGATIVE
    mov     byte [rbx + GS_PENDING + PENDING_DWARF],    0xFF

.mw_skip:
    ; commit flags
    mov     [rbx + GS_FLAGS], r9d

    pop     rbx
    pop     rbp
    ret