; ==============================================================
; wonder.asm — World Wonder construction system
;
; Three mega-projects that win the game when all complete.
; No struct changes: state packed into prestige._pad (uint32):
;   bits  [7:0]  = active wonder index (0-2) or WONDER_NONE (0xFF)
;   bits [23:8]  = ticks remaining
; Completion: upgrades.tier2 bits [42:40], one bit per wonder.
; ==============================================================
bits 64
%include "core/offsets.inc"

section .data

wonder_min_dwarves: db WONDER_THRONE_MIN_DWF, WONDER_DRILL_MIN_DWF, WONDER_HAMMER_MIN_DWF
wonder_build_ticks: dw WONDER_THRONE_TICKS,   WONDER_DRILL_TICKS,   WONDER_HAMMER_TICKS

section .text
global asm_start_wonder
global asm_tick_wonder
global asm_cancel_wonder

; ==============================================================
; asm_start_wonder(state=rdi, wonder_idx=rsi) → rax (1=ok, 0=fail)
; Checks resources + dwarves, deducts cost, begins construction.
; ==============================================================
asm_start_wonder:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    sub     rsp, 8

    mov     rbx, rdi
    mov     r12d, esi           ; r12d = wonder index (0-2)

    ; bounds check
    cmp     r12d, WONDER_COUNT
    jge     .fail

    ; already complete?
    mov     rax, [rbx + GS_UPGR_TIER2]
    mov     rcx, r12
    add     rcx, WONDER_BIT_BASE
    bt      rax, rcx
    jc      .fail

    ; already building something?
    mov     eax, dword [rbx + GS_PRESTIGE + PRESTIGE_PAD]
    and     eax, 0xFF
    cmp     eax, WONDER_NONE
    jne     .fail

    ; count alive dwarves
    lea     rdi, [rbx + GS_DWARVES]
    xor     r13d, r13d
    xor     ecx, ecx
.count_loop:
    cmp     ecx, MAX_DWARVES
    jge     .count_done
    movzx   edx, byte [rdi + DWARF_ALIVE]
    add     r13d, edx
    add     rdi, SIZEOF_DWARF
    inc     ecx
    jmp     .count_loop
.count_done:
    lea     rcx, [rel wonder_min_dwarves]
    movzx   eax, byte [rcx + r12]
    cmp     r13d, eax
    jl      .fail               ; not enough dwarves

    ; check + deduct resources per wonder
    cmp     r12d, WONDER_THRONE
    je      .throne_check
    cmp     r12d, WONDER_DRILL
    je      .drill_check
    jmp     .hammer_check

    ; ---- Throne of the Mountain King ----
.throne_check:
    cmp     qword [rbx + GS_RESOURCES + RES_GOLD],      WONDER_THRONE_GOLD
    jl      .fail
    cmp     qword [rbx + GS_RESOURCES + RES_STONE],     WONDER_THRONE_STONE
    jl      .fail
    cmp     qword [rbx + GS_RESOURCES + RES_IRON_BARS], WONDER_THRONE_BARS
    jl      .fail
    cmp     qword [rbx + GS_RESOURCES + RES_GEMS],      WONDER_THRONE_GEMS
    jl      .fail
    sub     qword [rbx + GS_RESOURCES + RES_GOLD],      WONDER_THRONE_GOLD
    sub     qword [rbx + GS_RESOURCES + RES_STONE],     WONDER_THRONE_STONE
    sub     qword [rbx + GS_RESOURCES + RES_IRON_BARS], WONDER_THRONE_BARS
    sub     qword [rbx + GS_RESOURCES + RES_GEMS],      WONDER_THRONE_GEMS
    jmp     .start_build

    ; ---- World Drill ----
.drill_check:
    cmp     qword [rbx + GS_RESOURCES + RES_GOLD],      WONDER_DRILL_GOLD
    jl      .fail
    cmp     qword [rbx + GS_RESOURCES + RES_STONE],     WONDER_DRILL_STONE
    jl      .fail
    cmp     qword [rbx + GS_RESOURCES + RES_IRON_BARS], WONDER_DRILL_BARS
    jl      .fail
    cmp     qword [rbx + GS_RESOURCES + RES_GEMS],      WONDER_DRILL_GEMS
    jl      .fail
    cmp     qword [rbx + GS_RESOURCES + RES_CRYSTALS],  WONDER_DRILL_CRYSTALS
    jl      .fail
    sub     qword [rbx + GS_RESOURCES + RES_GOLD],      WONDER_DRILL_GOLD
    sub     qword [rbx + GS_RESOURCES + RES_STONE],     WONDER_DRILL_STONE
    sub     qword [rbx + GS_RESOURCES + RES_IRON_BARS], WONDER_DRILL_BARS
    sub     qword [rbx + GS_RESOURCES + RES_GEMS],      WONDER_DRILL_GEMS
    sub     qword [rbx + GS_RESOURCES + RES_CRYSTALS],  WONDER_DRILL_CRYSTALS
    jmp     .start_build

    ; ---- Divine Hammer ----
.hammer_check:
    cmp     qword [rbx + GS_RESOURCES + RES_GOLD],      WONDER_HAMMER_GOLD
    jl      .fail
    cmp     qword [rbx + GS_RESOURCES + RES_STONE],     WONDER_HAMMER_STONE
    jl      .fail
    cmp     qword [rbx + GS_RESOURCES + RES_IRON_BARS], WONDER_HAMMER_BARS
    jl      .fail
    cmp     qword [rbx + GS_RESOURCES + RES_RELICS],    WONDER_HAMMER_RELICS
    jl      .fail
    cmp     qword [rbx + GS_RESOURCES + RES_CRYSTALS],  WONDER_HAMMER_CRYSTALS
    jl      .fail
    sub     qword [rbx + GS_RESOURCES + RES_GOLD],      WONDER_HAMMER_GOLD
    sub     qword [rbx + GS_RESOURCES + RES_STONE],     WONDER_HAMMER_STONE
    sub     qword [rbx + GS_RESOURCES + RES_IRON_BARS], WONDER_HAMMER_BARS
    sub     qword [rbx + GS_RESOURCES + RES_RELICS],    WONDER_HAMMER_RELICS
    sub     qword [rbx + GS_RESOURCES + RES_CRYSTALS],  WONDER_HAMMER_CRYSTALS

.start_build:
    ; Pack _pad: active=r12d, timer=build_ticks[r12]
    lea     rcx, [rel wonder_build_ticks]
    movzx   eax, word [rcx + r12*2]   ; ticks
    shl     eax, 8                     ; shift to bits [23:8]
    or      eax, r12d                  ; active in bits [7:0]
    mov     dword [rbx + GS_PRESTIGE + PRESTIGE_PAD], eax

    mov     rax, 1
    jmp     .done

.fail:
    xor     rax, rax

.done:
    add     rsp, 8
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

; ==============================================================
; asm_tick_wonder(state=rdi) — called every game tick
; Counts alive dwarves; if >= minimum, decrements timer.
; On reaching 0: sets the completion bit and clears _pad.
; Construction stalls (no decrement) if dwarves drop too low.
; ==============================================================
asm_tick_wonder:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13

    mov     rbx, rdi

    ; Load _pad and check if building
    mov     eax, dword [rbx + GS_PRESTIGE + PRESTIGE_PAD]
    movzx   r12d, al                    ; r12d = active wonder index
    cmp     r12d, WONDER_NONE
    je      .done
    cmp     r12d, WONDER_COUNT
    jge     .done                       ; corrupt index guard

    ; Count alive dwarves
    lea     rdi, [rbx + GS_DWARVES]
    xor     r13d, r13d
    xor     ecx, ecx
.alive_loop:
    cmp     ecx, MAX_DWARVES
    jge     .alive_done
    movzx   edx, byte [rdi + DWARF_ALIVE]
    add     r13d, edx
    add     rdi, SIZEOF_DWARF
    inc     ecx
    jmp     .alive_loop
.alive_done:
    lea     rcx, [rel wonder_min_dwarves]
    movzx   ecx, byte [rcx + r12]
    cmp     r13d, ecx
    jl      .done                       ; stall — not enough dwarves

    ; Decrement timer (bits [23:8] of _pad)
    mov     ecx, eax
    shr     ecx, 8
    and     ecx, 0xFFFF                 ; ecx = ticks remaining
    dec     ecx
    jz      .complete

    ; Store updated timer
    and     eax, 0xFF                   ; keep active index
    shl     ecx, 8
    or      eax, ecx
    mov     dword [rbx + GS_PRESTIGE + PRESTIGE_PAD], eax
    jmp     .done

.complete:
    ; Set completion bit in tier2
    mov     rax, [rbx + GS_UPGR_TIER2]
    mov     rcx, r12
    add     rcx, WONDER_BIT_BASE
    bts     rax, rcx
    mov     [rbx + GS_UPGR_TIER2], rax

    ; Clear _pad (no active wonder)
    mov     dword [rbx + GS_PRESTIGE + PRESTIGE_PAD], WONDER_NONE

    ; Push milestone event for the completed wonder
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x4B
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_MILESTONE
    mov     byte [rbx + GS_PENDING + PENDING_DWARF],    0xFF

.done:
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

; ==============================================================
; asm_cancel_wonder(state=rdi)
; Cancels the active wonder (no resource refund).
; ==============================================================
asm_cancel_wonder:
    movzx   eax, byte [rdi + GS_PRESTIGE + PRESTIGE_PAD]
    cmp     eax, WONDER_NONE
    je      .already_clear
    mov     dword [rdi + GS_PRESTIGE + PRESTIGE_PAD], WONDER_NONE
.already_clear:
    ret