; =========================================================
; asm/core/craft.asm - Craftsdwarf production system
;
; Exports:
;   asm_craft_assign(state, recipe_id, delta) -> rax (new assigned count)
;   asm_tick_craft(state)                     -> void
;
; Each recipe has a CraftSlot: assigned dwarves, active flag, timer.
; Each tick:
;   - If assigned>0 and not active: try to start (consume inputs)
;   - If active: decrement timer; on 0 deliver output, try restart
;
; Recipe table (10 bytes/entry):
;   [0] tier  [1] dwarves_per_unit  [2-3] timer_ticks(u16)
;   [4] in1_rid  [5] in1_amt  [6] in2_rid (0xFF=none)  [7] in2_amt
;   [8] out_rid  [9] out_amt
;
; Resource index (rid) = RES_x / 8 (converts offset to array index)
; Equipment outputs use rid >= 0x80 (mapped to resource slots)
; =========================================================

%include "core/offsets.inc"

%define RES_NONE    0xFF

; Resource indices (offset / 8)
%define RID_GOLD        0
%define RID_STONE       1
%define RID_WOOD        2
%define RID_FOOD        3
%define RID_MANA        4
%define RID_IRON_ORE    5
%define RID_GEMS        6
%define RID_RELICS      7
%define RID_CRYSTALS    8
%define RID_IRON_BARS   9
%define RID_ALE         10

; Equipment output codes (>= 0x80, stored in relics/crystals/iron_bars slots temporarily)
%define RID_WEAPON      0x80    ; stored at RID_RELICS
%define RID_ARMOUR      0x81    ; stored at RID_CRYSTALS
%define RID_TOOL        0x82    ; stored at RID_IRON_BARS+1 (future dedicated slot)

section .data

recipe_table:
    ; [0] RECIPE_IRON_BARS_I  -- tier 1, 1 dwarf/unit, 5 ticks, 2 iron ore -> 1 iron bar
    db 1, 1, 5, 0,  RID_IRON_ORE, 2, RES_NONE, 0,  RID_IRON_BARS, 1
    ; [1] RECIPE_ALE_I        -- tier 1, 1 dwarf/unit, 3 ticks, 1 food + 1 wood -> 1 ale
    db 1, 1, 3, 0,  RID_FOOD, 1, RID_WOOD, 1,       RID_ALE, 1
    ; [2] RECIPE_IRON_BARS_II -- tier 2, 1 dwarf/unit, 5 ticks, 3 iron ore -> 2 iron bars
    db 2, 1, 5, 0,  RID_IRON_ORE, 3, RES_NONE, 0,  RID_IRON_BARS, 2
    ; [3] RECIPE_WEAPONS_I    -- tier 2, 2 dwarves/unit, 10 ticks, 3 iron bars -> 1 weapon
    db 2, 2, 10, 0, RID_IRON_BARS, 3, RES_NONE, 0,  RID_WEAPON, 1
    ; [4] RECIPE_ARMOUR_I     -- tier 2, 2 dwarves/unit, 15 ticks, 4 iron bars -> 1 armour
    db 2, 2, 15, 0, RID_IRON_BARS, 4, RES_NONE, 0,  RID_ARMOUR, 1
    ; [5] RECIPE_TOOLS_I      -- tier 2, 2 dwarves/unit, 10 ticks, 2 bars+1 gem -> 1 tool
    db 2, 2, 10, 0, RID_IRON_BARS, 2, RID_GEMS, 1,  RID_TOOL, 1

%define RSIZ    10  ; recipe entry size

section .text
    global asm_craft_assign
    global asm_tick_craft

; ---------------------------------------------------------
; rid_to_offset(ecx=rid) -> rax = byte offset into GS_RESOURCES
; Equipment rids are mapped to temp resource slots
; ---------------------------------------------------------
%macro RID_TO_OFF 0
    cmp     ecx, 0x80
    jl      %%normal
    sub     ecx, 0x80           ; 0=weapon,1=armour,2=tool
    ; map: weapon->relics(7), armour->crystals(8), tool->(no slot yet, skip)
    lea     rax, [rel equip_map]
    movzx   ecx, byte [rax + rcx]
%%normal:
    mov     eax, ecx
    imul    eax, 8              ; byte offset
%endmacro

equip_map: db RID_RELICS, RID_CRYSTALS, 0xFF   ; weapon, armour, tool(unused)

; ---------------------------------------------------------
; count_free_craftsdwarves(rbx=state) -> ecx
; ---------------------------------------------------------
count_free_craftsdwarves:
    xor     ecx, ecx
    lea     rax, [rbx + GS_DWARVES]
    mov     edx, MAX_DWARVES
.cfl:
    test    edx, edx
    jz      .cfd
    movzx   r8d, byte [rax + DWARF_ALIVE]
    test    r8d, r8d
    jz      .cfn
    movzx   r8d, byte [rax + DWARF_JOB]
    cmp     r8d, JOB_CRAFTSDWARF
    jne     .cfn
    inc     ecx
.cfn:
    add     rax, SIZEOF_DWARF
    dec     edx
    jmp     .cfl
.cfd:
    ; subtract already assigned
    lea     rax, [rbx + GS_CRAFT]
    mov     edx, RECIPE_COUNT
.csl:
    test    edx, edx
    jz      .csd
    movzx   r8d, byte [rax + CRAFT_ASSIGNED]
    sub     ecx, r8d
    add     rax, SIZEOF_CRAFTSLOT
    dec     edx
    jmp     .csl
.csd:
    ret

; ---------------------------------------------------------
; asm_craft_assign(rdi=state, rsi=recipe_id, rdx=delta) -> rax
; delta +1 = add dwarf, -1 = remove dwarf
; ---------------------------------------------------------
asm_craft_assign:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi
    movzx   r12d, sil       ; recipe_id
    mov     r13d, edx       ; delta

    cmp     r12d, RECIPE_COUNT
    jge     .fail

    ; slot ptr
    lea     r14, [rbx + GS_CRAFT]
    imul    rax, r12, SIZEOF_CRAFTSLOT
    add     r14, rax

    cmp     r13d, 0
    jl      .remove

    ; Adding — need a free craftsdwarf
    call    count_free_craftsdwarves    ; ecx = free count; clobbers rax
    test    ecx, ecx
    jz      .fail
    movzx   eax, byte [r14 + CRAFT_ASSIGNED]  ; reload after call
    inc     eax
    mov     byte [r14 + CRAFT_ASSIGNED], al
    jmp     .ok

.remove:
    movzx   eax, byte [r14 + CRAFT_ASSIGNED]
    test    eax, eax
    jz      .fail
    dec     eax
    mov     byte [r14 + CRAFT_ASSIGNED], al
    test    eax, eax
    jnz     .ok
    mov     byte [r14 + CRAFT_ACTIVE], 0
    mov     word [r14 + CRAFT_TIMER],  0

.ok:
    movzx   eax, byte [r14 + CRAFT_ASSIGNED]
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
; try_start(rbx=state, r14=slot, r15=recipe_entry) -> eax = output_count (0=fail)
; Checks inputs, consumes them, starts timer
; ---------------------------------------------------------
try_start:
    push    rcx
    push    rdx
    push    r8
    push    r9
    push    r10

    ; output_count = floor(assigned / dwarves_per_unit)
    movzx   eax, byte [r14 + CRAFT_ASSIGNED]
    movzx   ecx, byte [r15 + 1]
    test    ecx, ecx
    jz      .ts_fail
    xor     edx, edx
    div     ecx
    test    eax, eax
    jz      .ts_fail
    mov     r9d, eax            ; output_count

    ; Check in1: need in1_amt * output_count
    movzx   r8d, byte [r15 + 4]    ; in1_rid
    movzx   ecx, byte [r15 + 5]    ; in1_amt
    imul    ecx, r9d
    mov     eax, r8d
    imul    eax, 8
    movsx   rcx, ecx
    cmp     [rbx + GS_RESOURCES + rax], rcx
    jl      .ts_fail

    ; Check in2 (if any)
    movzx   r8d, byte [r15 + 6]
    cmp     r8d, 0xFF
    je      .ts_skip2
    movzx   ecx, byte [r15 + 7]
    imul    ecx, r9d
    mov     eax, r8d
    imul    eax, 8
    movsx   rcx, ecx
    cmp     [rbx + GS_RESOURCES + rax], rcx
    jl      .ts_fail
.ts_skip2:

    ; Consume in1
    movzx   r8d, byte [r15 + 4]
    movzx   ecx, byte [r15 + 5]
    imul    ecx, r9d
    mov     eax, r8d
    imul    eax, 8
    movsx   rcx, ecx
    sub     [rbx + GS_RESOURCES + rax], rcx

    ; Consume in2
    movzx   r8d, byte [r15 + 6]
    cmp     r8d, 0xFF
    je      .ts_produce
    movzx   ecx, byte [r15 + 7]
    imul    ecx, r9d
    mov     eax, r8d
    imul    eax, 8
    movsx   rcx, ecx
    sub     [rbx + GS_RESOURCES + rax], rcx

.ts_produce:
    ; Start timer
    movzx   ecx, byte [r15 + 2]
    mov     word [r14 + CRAFT_TIMER], cx
    mov     byte [r14 + CRAFT_ACTIVE], 1
    mov     eax, r9d            ; return output_count
    jmp     .ts_done

.ts_fail:
    xor     eax, eax
    mov     byte [r14 + CRAFT_ACTIVE], 0

.ts_done:
    pop     r10
    pop     r9
    pop     r8
    pop     rdx
    pop     rcx
    ret

; ---------------------------------------------------------
; deliver(rbx=state, r15=recipe_entry, r10d=output_count)
; Adds output to the appropriate resource slot
; ---------------------------------------------------------
deliver:
    push    rax
    push    rcx
    push    rdx

    movzx   ecx, byte [r15 + 8]     ; out_rid
    movzx   edx, byte [r15 + 9]     ; out_amt
    imul    edx, r10d               ; total output

    ; Equipment?
    cmp     ecx, 0x80
    jl      .del_res
    sub     ecx, 0x80
    lea     rax, [rel equip_map]
    movzx   ecx, byte [rax + rcx]
    cmp     ecx, 0xFF
    je      .del_skip               ; tool has no slot yet

.del_res:
    mov     eax, ecx
    imul    eax, 8
    movsx   rdx, edx
    add     [rbx + GS_RESOURCES + rax], rdx

.del_skip:
    pop     rdx
    pop     rcx
    pop     rax
    ret

; ---------------------------------------------------------
; asm_tick_craft(rdi=state) -> void
; ---------------------------------------------------------
asm_tick_craft:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi
    xor     r13d, r13d          ; r13 = recipe index

.loop:
    cmp     r13d, RECIPE_COUNT
    jge     .done

    ; slot = &craft[r13]
    lea     r14, [rbx + GS_CRAFT]
    imul    rax, r13, SIZEOF_CRAFTSLOT
    add     r14, rax

    ; recipe entry = recipe_table + r13 * RSIZ
    lea     r15, [rel recipe_table]
    imul    rax, r13, RSIZ
    add     r15, rax

    ; Skip if no dwarves assigned
    movzx   eax, byte [r14 + CRAFT_ASSIGNED]
    test    eax, eax
    jz      .next

    movzx   eax, byte [r14 + CRAFT_ACTIVE]
    test    eax, eax
    jz      .start              ; assigned but idle — try to start

    ; Active: decrement timer
    movzx   eax, word [r14 + CRAFT_TIMER]
    test    eax, eax
    jz      .complete           ; already 0
    dec     eax
    mov     word [r14 + CRAFT_TIMER], ax
    test    eax, eax
    jnz     .next               ; still ticking

.complete:
    ; Timer hit 0 — compute output_count and deliver
    movzx   eax, byte [r14 + CRAFT_ASSIGNED]
    movzx   ecx, byte [r15 + 1]     ; dwarves_per_unit
    test    ecx, ecx
    jz      .start
    xor     edx, edx
    div     ecx
    mov     r10d, eax           ; r10 = output_count

    call    deliver

    ; Deactivate before restart attempt
    mov     byte [r14 + CRAFT_ACTIVE], 0

.start:
    call    try_start           ; tries to consume inputs and restart timer
    jmp     .next

.next:
    inc     r13d
    jmp     .loop

.done:
    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret