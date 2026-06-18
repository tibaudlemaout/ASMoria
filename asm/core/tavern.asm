; =========================================================
; asm/core/tavern.asm - Tavern buff system
;
; Exports:
;   asm_tavern_activate(state, buff_id) -> rax (1=ok, 0=fail)
;     Activates a buff if a free slot exists and ale cost met.
;
;   asm_tick_tavern(state) -> void
;     Decrements all active buff timers. Applies per-tick
;     effects (lumberjack song, harvest festival, clarity, focus).
;     Removes expired buffs.
;
; Buff costs and durations are defined in the buff_table below.
; One-shot effects (Toast, Feast morale) are applied on activation.
; Ongoing effects are checked each tick by reading active buff slots.
;
; STACK: entry 8 mod 16
;   push rbp(16) rbx(24) r12(32) r13(40) r14(48) r15(56)
;   sub rsp,8(64) = aligned
; =========================================================

%include "core/offsets.inc"

; Buff table entry (8 bytes):
;   [0] ale_cost (uint16)
;   [2] duration (uint16) ticks
;   [4] flags    (uint8)  BTFL_* bitmask
;   [5..7] pad
%define BTFL_YIELD_PLUS     0x01    ; +20% yield all dwarves
%define BTFL_YIELD_MINUS    0x02    ; -10% yield
%define BTFL_FATIGUE_SLOW   0x04    ; fatigue gen halved
%define BTFL_FATIGUE_FAST   0x08    ; +5 fatigue/tick (feast side effect)
%define BTFL_XP_PLUS        0x10    ; +1 XP/tick all

section .data

; Buff cost/duration table indexed by buff_id (1..10)
; Entry: ale_cost(u16), duration(u16), flags(u8), pad(3)
buff_table:
    ; [0] BUFF_NONE placeholder
    dw 0, 0
    db 0, 0, 0, 0

    ; [1] BUFF_FEAST: 200 ale, 100 ticks, +20% yield, +5 fat/tick
    dw 200, 100
    db (BTFL_YIELD_PLUS | BTFL_FATIGUE_FAST), 0, 0, 0

    ; [2] BUFF_DRINKING_CONTEST: 150 ale, 100 ticks, -10% yield, half fatigue
    dw 150, 100
    db (BTFL_YIELD_MINUS | BTFL_FATIGUE_SLOW), 0, 0, 0

    ; [3] BUFF_TOAST: 100 ale, 100 ticks, +1 XP/tick (morale restored on activate)
    dw 100, 100
    db BTFL_XP_PLUS, 0, 0, 0

    ; [4] BUFF_LONG_REST: 80 ale, 80 ticks, fat recovery 2x, morale decay halved
    dw 80, 80
    db BTFL_FATIGUE_SLOW, 0, 0, 0

    ; [5] BUFF_MINERS_COURAGE: 80 ale, 60 ticks (depth fatigue ignored — checked in dwarves.asm)
    dw 80, 60
    db 0, 0, 0, 0

    ; [6] BUFF_LUMBERJACKS_SONG: 70 ale, 60 ticks (+2 wood/tick per lumberer — applied in tick)
    dw 70, 60
    db 0, 0, 0, 0

    ; [7] BUFF_HARVEST_FESTIVAL: 70 ale, 60 ticks (+2 food/tick per farmer — applied in tick)
    dw 70, 60
    db 0, 0, 0, 0

    ; [8] BUFF_WARRIORS_DRAFT: 100 ale, 80 ticks (+8 ATK, +15 HP — checked in breach.asm)
    dw 100, 80
    db 0, 0, 0, 0

    ; [9] BUFF_SCHOLARS_CLARITY: 90 ale, 80 ticks (+50% mana from scholars — applied in infra.asm)
    dw 90, 80
    db 0, 0, 0, 0

    ; [10] BUFF_CRAFTERS_FOCUS: 80 ale, 60 ticks (craft timers 2x speed — checked in craft.asm)
    dw 80, 60
    db 0, 0, 0, 0

%define BTAB_ENTRY_SIZE 8

; Helper: get max buff slots from tavern level
; tavern_level 1->1, 2->2, 3->3
; (simply equals tavern_level, clamped to MAX_TAVERN_BUFFS)

section .text
    global asm_tavern_activate
    global asm_tick_tavern
    global asm_tavern_buff_active

; ---------------------------------------------------------
; asm_tavern_buff_active(rdi=state, rsi=buff_id) -> rax (1=active, 0=not)
; Checks if a specific buff is currently active in any slot
; ---------------------------------------------------------
asm_tavern_buff_active:
    lea     rcx, [rdi + GS_TAVERN_BUFFS]
    mov     edx, MAX_TAVERN_BUFFS
.chk:
    test    edx, edx
    jz      .not_active
    movzx   eax, byte [rcx + TBUFF_ACTIVE]
    test    eax, eax
    jz      .chk_next
    movzx   eax, byte [rcx + TBUFF_ID]
    cmp     eax, esi
    je      .is_active
.chk_next:
    add     rcx, SIZEOF_TAVAVERBUFF
    dec     edx
    jmp     .chk
.is_active:
    mov     eax, 1
    ret
.not_active:
    xor     eax, eax
    ret

; ---------------------------------------------------------
; asm_tavern_activate(rdi=state, rsi=buff_id) -> rax
; ---------------------------------------------------------
asm_tavern_activate:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi
    movzx   r12d, sil           ; buff_id

    ; validate
    cmp     r12d, 1
    jl      .fail
    cmp     r12d, BUFF_COUNT
    jge     .fail

    ; tavern must be built
    cmp     dword [rbx + GS_TAVERN_LEVEL], 0
    je      .fail

    ; check already active
    mov     rdi, rbx
    mov     rsi, r12
    call    asm_tavern_buff_active
    test    eax, eax
    jnz     .fail               ; already running

    ; find free slot (within tavern_level count)
    mov     r13d, [rbx + GS_TAVERN_LEVEL]
    cmp     r13d, MAX_TAVERN_BUFFS
    jle     .ok_level
    mov     r13d, MAX_TAVERN_BUFFS
.ok_level:
    lea     r14, [rbx + GS_TAVERN_BUFFS]
    xor     r15d, r15d
.find_slot:
    cmp     r15d, r13d
    jge     .fail               ; no free slot within level limit
    movzx   eax, byte [r14 + TBUFF_ACTIVE]
    test    eax, eax
    jz      .found_slot
    add     r14, SIZEOF_TAVAVERBUFF
    inc     r15d
    jmp     .find_slot
.found_slot:

    ; get buff table entry
    lea     rax, [rel buff_table]
    imul    rcx, r12, BTAB_ENTRY_SIZE
    add     rax, rcx            ; rax = &buff_table[buff_id]

    ; check ale cost
    movzx   ecx, word [rax + 0]     ; ale_cost
    movsx   rcx, ecx
    cmp     [rbx + GS_RESOURCES + RES_ALE], rcx
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_ALE], rcx

    ; activate slot
    mov     byte [r14 + TBUFF_ACTIVE], 1
    mov     byte [r14 + TBUFF_ID],     r12b
    movzx   ecx, word [rax + 2]     ; duration
    mov     word [r14 + TBUFF_TIMER], cx

    ; one-shot effects on activation
    cmp     r12d, BUFF_TOAST
    jne     .check_feast_morale
    ; Toast: restore all dwarves morale to 80
    lea     rdi, [rbx + GS_DWARVES]
    mov     ecx, MAX_DWARVES
.toast_loop:
    test    ecx, ecx
    jz      .one_shot_done
    movzx   eax, byte [rdi + DWARF_ALIVE]
    test    eax, eax
    jz      .toast_next
    mov     byte [rdi + DWARF_MORALE], 80
.toast_next:
    add     rdi, SIZEOF_DWARF
    dec     ecx
    jmp     .toast_loop

.check_feast_morale:
    ; (Feast has no one-shot morale effect, just ongoing yield+fatigue)

.one_shot_done:
    mov     eax, 1
    jmp     .done
.fail:
    xor     eax, eax
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
; asm_tick_tavern(rdi=state) -> void
; Ticks all active buffs, applies per-tick effects, expires
; ---------------------------------------------------------
asm_tick_tavern:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi

    ; Skip if no tavern
    cmp     dword [rbx + GS_TAVERN_LEVEL], 0
    je      .done

    lea     r12, [rbx + GS_TAVERN_BUFFS]
    xor     r13d, r13d          ; slot index

.slot_loop:
    cmp     r13d, MAX_TAVERN_BUFFS
    jge     .done

    lea     r14, [r12 + r13 * SIZEOF_TAVAVERBUFF]

    movzx   eax, byte [r14 + TBUFF_ACTIVE]
    test    eax, eax
    jz      .slot_next

    ; decrement timer
    movzx   eax, word [r14 + TBUFF_TIMER]
    test    eax, eax
    jz      .expire
    dec     eax
    mov     word [r14 + TBUFF_TIMER], ax
    test    eax, eax
    jz      .expire

    ; apply per-tick effects based on buff_id
    movzx   r15d, byte [r14 + TBUFF_ID]

    cmp     r15d, BUFF_LUMBERJACKS_SONG
    je      .lumberjack_song

    cmp     r15d, BUFF_HARVEST_FESTIVAL
    je      .harvest_festival

    jmp     .slot_next

.lumberjack_song:
    ; +2 wood per alive lumberer
    lea     rax, [rbx + GS_DWARVES]
    mov     ecx, MAX_DWARVES
.lumb_loop:
    test    ecx, ecx
    jz      .slot_next
    movzx   edx, byte [rax + DWARF_ALIVE]
    test    edx, edx
    jz      .lumb_next
    movzx   edx, byte [rax + DWARF_JOB]
    cmp     dl, JOB_LUMBERER
    jne     .lumb_next
    add     qword [rbx + GS_RESOURCES + RES_WOOD], 2
.lumb_next:
    add     rax, SIZEOF_DWARF
    dec     ecx
    jmp     .lumb_loop

.harvest_festival:
    ; +2 food per alive farmer
    lea     rax, [rbx + GS_DWARVES]
    mov     ecx, MAX_DWARVES
.farm_loop:
    test    ecx, ecx
    jz      .slot_next
    movzx   edx, byte [rax + DWARF_ALIVE]
    test    edx, edx
    jz      .farm_next
    movzx   edx, byte [rax + DWARF_JOB]
    cmp     dl, JOB_FARMER
    jne     .farm_next
    add     qword [rbx + GS_RESOURCES + RES_FOOD], 2
.farm_next:
    add     rax, SIZEOF_DWARF
    dec     ecx
    jmp     .farm_loop

.expire:
    mov     byte [r14 + TBUFF_ACTIVE], 0
    mov     byte [r14 + TBUFF_ID],     0
    mov     word [r14 + TBUFF_TIMER],  0

.slot_next:
    inc     r13d
    jmp     .slot_loop

.done:
    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret