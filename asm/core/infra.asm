; =========================================================
; asm/core/infra.asm - Infrastructure per-tick effects
;
; Called once per tick by tick.asm.
;
; Effects:
;   Mana Well (id=7): +2 mana per level per tick (passive)
;
;   Rune Halls (id=6):
;     Lv1: Scholars unlocked (checked in jobs.asm)
;     Lv2: Each alive Scholar adds +1 mana/tick
;     Lv3: All working dwarves get +1 XP/tick bonus
;     Lv4: Each alive Scholar adds +2 mana/tick total
;     Lv5: Research unlocked (future)
;
;   Watch Tower (id=5):
;     Lv1: Guards unlocked (checked in jobs.asm)
;     Lv2+: Guard protection (applied in stat_events.asm)
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  push rbx: 24
;   2 pushes, NO calls — no sub rsp needed.
; =========================================================

%include "core/offsets.inc"

section .text
    global asm_tick_infra

asm_tick_infra:
    push    rbp
    mov     rbp, rsp
    push    rbx

    mov     rbx, rdi
    mov     r8,  [rbx + GS_UPGR_TIER1]
    mov     r9d, [rbx + GS_FLAGS]       ; degraded flags

    ; -------------------------------------------------------
    ; Mana Well: +2 mana per level per tick (skip if degraded)
    ; -------------------------------------------------------
    test    r9d, (1 << 2)               ; FLAG_MANA_DEGRADED
    jnz     .check_rune

    mov     rax, r8
    shr     rax, (UPGR_MANA_WELL * 4)
    and     rax, 0xF                    ; mana well level
    test    rax, rax
    jz      .check_rune

    imul    rax, 2                      ; +2 per level
    add     [rbx + GS_RESOURCES + RES_MANA], rax

    ; -------------------------------------------------------
    ; Rune Halls: Scholar effects
    ; -------------------------------------------------------
.check_rune:
    test    r9d, (1 << 1)               ; FLAG_RUNE_DEGRADED
    jnz     .check_xp_bonus             ; skip if degraded

    mov     rax, r8
    shr     rax, (UPGR_RUNE_HALLS * 4)
    and     rax, 0xF                    ; rune halls level
    cmp     rax, 2
    jl      .check_xp_bonus             ; lv1 = unlock only

    ; lv2+: count alive scholars, add mana
    ; lv4+: double mana per scholar
    mov     r9, rax                     ; r9 = rune level
    xor     r10, r10                    ; r10 = scholar count
    lea     r11, [rbx + GS_DWARVES]
    mov     rcx, MAX_DWARVES

.scholar_count:
    test    rcx, rcx
    jz      .apply_scholar_mana
    movzx   edx, byte [r11 + DWARF_ALIVE]
    test    dl, dl
    jz      .scholar_next
    movzx   edx, byte [r11 + DWARF_JOB]
    cmp     dl, JOB_SCHOLAR
    jne     .scholar_next
    inc     r10
.scholar_next:
    add     r11, SIZEOF_DWARF
    dec     rcx
    jmp     .scholar_count

.apply_scholar_mana:
    test    r10, r10
    jz      .check_xp_bonus

    ; mana per scholar: 1 at lv2-3, 2 at lv4-5
    mov     rax, 1
    cmp     r9, 4
    jl      .add_mana
    mov     rax, 2
.add_mana:
    imul    rax, r10                    ; mana * scholar_count
    add     [rbx + GS_RESOURCES + RES_MANA], rax

    ; -------------------------------------------------------
    ; Rune Halls lv3+: +1 XP/tick to all working dwarves
    ; -------------------------------------------------------
.check_xp_bonus:
    mov     rax, r8
    shr     rax, (UPGR_RUNE_HALLS * 4)
    and     rax, 0xF
    cmp     rax, 3
    jl      .done

    ; add 1 XP to job_xp[current_job] for each working dwarf
    lea     r11, [rbx + GS_DWARVES]
    mov     rcx, MAX_DWARVES

.xp_loop:
    test    rcx, rcx
    jz      .done
    movzx   edx, byte [r11 + DWARF_ALIVE]
    test    dl, dl
    jz      .xp_next
    movzx   edx, byte [r11 + DWARF_JOB]
    test    dl, dl
    jz      .xp_next                    ; idle: no bonus
    movzx   eax, byte [r11 + DWARF_PREV_JOB]
    test    al, al
    jnz     .xp_next                    ; force-resting: no bonus

    ; add 1 XP to job_xp[job]
    movzx   rax, dl
    shl     rax, 3
    add     qword [r11 + DWARF_JOB_XP + rax], 1

.xp_next:
    add     r11, SIZEOF_DWARF
    dec     rcx
    jmp     .xp_loop

    ; -------------------------------------------------------
    ; Prestige bonuses: passive food/wood ticks
    ; -------------------------------------------------------
    mov     rax, [rbx + GS_PRESTIGE + PRESTIGE_NODES]

    bt      rax, PNODE_FOOD_TICK
    jnc     .no_food_tick
    add     qword [rbx + GS_RESOURCES + RES_FOOD], 1
.no_food_tick:
    bt      rax, PNODE_WOOD_TICK
    jnc     .no_wood_tick
    add     qword [rbx + GS_RESOURCES + RES_WOOD], 1
.no_wood_tick:
    bt      rax, PNODE_MANA_TICK_1
    jnc     .no_mana_p1
    add     qword [rbx + GS_RESOURCES + RES_MANA], 1
.no_mana_p1:
    bt      rax, PNODE_MANA_TICK_2
    jnc     .no_mana_p2
    add     qword [rbx + GS_RESOURCES + RES_MANA], 1
.no_mana_p2:

.done:
    pop     rbx
    pop     rbp
    ret