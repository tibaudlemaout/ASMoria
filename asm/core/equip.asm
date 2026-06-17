; =========================================================
; asm/core/equip.asm - Dwarf equipment system
;
; Exports:
;   asm_equip(state, dwarf_idx, equip_type) -> rax (1=ok, 0=fail)
;     Equips an item to a dwarf. Consumes 1 from the matching
;     resource pool. If dwarf already has an item, it is returned
;     to the pool first (unequip then re-equip).
;
;   asm_unequip(state, dwarf_idx) -> rax (1=ok, 0=fail)
;     Removes equipment from dwarf, returns item to pool.
;
;   asm_get_equip_atk_bonus(state, dwarf_idx) -> rax (bonus ATK)
;     Returns ATK bonus from weapon (0 if none or wrong job).
;
;   asm_get_equip_hp_bonus(state, dwarf_idx) -> rax (bonus HP)
;     Returns HP bonus from armour (0 if none or wrong job).
;
;   asm_get_equip_yield_bonus(state, dwarf_idx) -> rax (bonus %)
;     Returns yield % bonus from tool (0 if none or wrong job).
;
; Bonus values (tunable):
;   Weapon -> +10 ATK per weapon equipped
;   Armour -> +20 HP per armour equipped
;   Tool   -> +15% yield per tool equipped
;
; STACK: entry 8 mod 16
;   push rbp(16) rbx(24) r12(32) r13(40) r14(48) r15(56)
;   sub rsp,8(64) = aligned
; =========================================================

%include "core/offsets.inc"

%define WEAPON_ATK_BONUS    10
%define ARMOUR_HP_BONUS     20
%define TOOL_YIELD_BONUS    15  ; percent

; Resource slot for each equipment type
%macro EQUIP_RES 1
    %if %1 == EQUIP_WEAPON
        mov eax, RES_WEAPONS
    %elif %1 == EQUIP_ARMOUR
        mov eax, RES_ARMOUR
    %else
        mov eax, RES_TOOLS
    %endif
%endmacro

section .text
    global asm_equip
    global asm_unequip
    global asm_get_equip_atk_bonus
    global asm_get_equip_hp_bonus
    global asm_get_equip_yield_bonus

; ---------------------------------------------------------
; equip_res_offset(ecx=equip_type) -> rax = RES_* offset
; ---------------------------------------------------------
equip_res_offset:
    cmp     ecx, EQUIP_WEAPON
    je      .weapon
    cmp     ecx, EQUIP_ARMOUR
    je      .armour
    cmp     ecx, EQUIP_TOOL
    je      .tool
    xor     eax, eax
    ret
.weapon:
    mov     eax, RES_WEAPONS
    ret
.armour:
    mov     eax, RES_ARMOUR
    ret
.tool:
    mov     eax, RES_TOOLS
    ret

; ---------------------------------------------------------
; asm_equip(rdi=state, rsi=dwarf_idx, rdx=equip_type) -> rax
; ---------------------------------------------------------
asm_equip:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi
    mov     r12, rsi            ; dwarf_idx
    mov     r13d, edx           ; equip_type

    ; validate
    cmp     r12, MAX_DWARVES
    jge     .fail
    cmp     r13d, EQUIP_NONE
    je      .fail
    cmp     r13d, EQUIP_TOOL
    jg      .fail

    ; dwarf ptr
    imul    r14, r12, SIZEOF_DWARF
    lea     r14, [rbx + GS_DWARVES + r14]

    ; must be alive
    movzx   eax, byte [r14 + DWARF_ALIVE]
    test    eax, eax
    jz      .fail

    ; if already equipped, return old item to pool
    movzx   eax, byte [r14 + DWARF_EQUIPMENT]
    test    eax, eax
    jz      .no_unequip
    ; return old item
    mov     ecx, eax
    call    equip_res_offset
    add     qword [rbx + GS_RESOURCES + rax], 1
.no_unequip:

    ; check new item available in pool
    mov     ecx, r13d
    call    equip_res_offset
    cmp     qword [rbx + GS_RESOURCES + rax], 1
    jl      .fail

    ; consume from pool and equip
    sub     qword [rbx + GS_RESOURCES + rax], 1
    mov     byte [r14 + DWARF_EQUIPMENT], r13b

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
; asm_unequip(rdi=state, rsi=dwarf_idx) -> rax
; ---------------------------------------------------------
asm_unequip:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi
    mov     r12, rsi

    cmp     r12, MAX_DWARVES
    jge     .fail

    imul    r14, r12, SIZEOF_DWARF
    lea     r14, [rbx + GS_DWARVES + r14]

    movzx   eax, byte [r14 + DWARF_ALIVE]
    test    eax, eax
    jz      .fail

    movzx   ecx, byte [r14 + DWARF_EQUIPMENT]
    cmp     ecx, EQUIP_NONE
    je      .fail               ; nothing equipped

    ; return to pool
    call    equip_res_offset
    add     qword [rbx + GS_RESOURCES + rax], 1
    mov     byte [r14 + DWARF_EQUIPMENT], EQUIP_NONE

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
; asm_get_equip_atk_bonus(rdi=state, rsi=dwarf_idx) -> rax
; Returns WEAPON_ATK_BONUS if dwarf has a weapon, else 0
; ---------------------------------------------------------
asm_get_equip_atk_bonus:
    cmp     rsi, MAX_DWARVES
    jge     .none
    imul    rax, rsi, SIZEOF_DWARF
    lea     rax, [rdi + GS_DWARVES + rax]
    movzx   ecx, byte [rax + DWARF_EQUIPMENT]
    cmp     ecx, EQUIP_WEAPON
    jne     .none
    mov     eax, WEAPON_ATK_BONUS
    ret
.none:
    xor     eax, eax
    ret

; ---------------------------------------------------------
; asm_get_equip_hp_bonus(rdi=state, rsi=dwarf_idx) -> rax
; Returns ARMOUR_HP_BONUS if dwarf has armour, else 0
; ---------------------------------------------------------
asm_get_equip_hp_bonus:
    cmp     rsi, MAX_DWARVES
    jge     .none
    imul    rax, rsi, SIZEOF_DWARF
    lea     rax, [rdi + GS_DWARVES + rax]
    movzx   ecx, byte [rax + DWARF_EQUIPMENT]
    cmp     ecx, EQUIP_ARMOUR
    jne     .none
    mov     eax, ARMOUR_HP_BONUS
    ret
.none:
    xor     eax, eax
    ret

; ---------------------------------------------------------
; asm_get_equip_yield_bonus(rdi=state, rsi=dwarf_idx) -> rax
; Returns TOOL_YIELD_BONUS % if dwarf has a tool, else 0
; Only applies to miners, lumberers, farmers
; ---------------------------------------------------------
asm_get_equip_yield_bonus:
    cmp     rsi, MAX_DWARVES
    jge     .none
    imul    rax, rsi, SIZEOF_DWARF
    lea     rax, [rdi + GS_DWARVES + rax]
    movzx   ecx, byte [rax + DWARF_EQUIPMENT]
    cmp     ecx, EQUIP_TOOL
    jne     .none
    ; only for resource-gathering jobs
    movzx   ecx, byte [rax + DWARF_JOB]
    cmp     ecx, JOB_MINER
    je      .has_bonus
    cmp     ecx, JOB_LUMBERER
    je      .has_bonus
    cmp     ecx, JOB_FARMER
    je      .has_bonus
    jmp     .none
.has_bonus:
    mov     eax, TOOL_YIELD_BONUS
    ret
.none:
    xor     eax, eax
    ret