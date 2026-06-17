; =========================================================
; asm/core/jobs.asm - Dwarf job assignment
;
; asm_assign_job(state, dwarf_idx, job)
;   rdi = state, rsi = dwarf index, rdx = new job
;   rax = 1 success, 0 failure
;
; Guard requires Watch Tower >= 1
; Scholar requires Rune Halls >= 1
;
; STACK: may push rdi,rsi,rcx,rdx,r8 in craft-unassign path (no calls made).
; =========================================================

%include "core/offsets.inc"

section .text
    global asm_assign_job

asm_assign_job:
    cmp     rsi, MAX_DWARVES
    jge     .fail

    imul    rax, rsi, SIZEOF_DWARF
    lea     rcx, [rdi + GS_DWARVES + rax]

    ; must be alive
    movzx   eax, byte [rcx + DWARF_ALIVE]
    test    al, al
    jz      .fail

    ; refuse if force-resting
    movzx   eax, byte [rcx + DWARF_PREV_JOB]
    test    al, al
    jnz     .fail

    ; check job unlock requirements
    cmp     dl, JOB_GUARD
    je      .check_guard
    cmp     dl, JOB_SCHOLAR
    je      .check_scholar
    cmp     dl, JOB_CRAFTSDWARF
    je      .check_workshop
    jmp     .assign

.check_workshop:
    mov     rax, [rdi + GS_UPGR_TIER1]
    shr     rax, (UPGR_WORKSHOP * 4)
    and     rax, 0xF
    test    rax, rax
    jz      .fail               ; Workshop not built
    jmp     .assign

.check_guard:
    mov     rax, [rdi + GS_UPGR_TIER1]
    shr     rax, (UPGR_WATCH_TOWER * 4)
    and     rax, 0xF
    test    rax, rax
    jz      .fail               ; Watch Tower not built
    jmp     .assign

.check_scholar:
    mov     rax, [rdi + GS_UPGR_TIER1]
    shr     rax, (UPGR_RUNE_HALLS * 4)
    and     rax, 0xF
    test    rax, rax
    jz      .fail               ; Rune Halls not built
    jmp     .assign

.assign:
    ; If dwarf was a Craftsdwarf, remove from any craft slot before changing job
    movzx   eax, byte [rcx + DWARF_JOB]
    cmp     eax, JOB_CRAFTSDWARF
    jne     .no_craft_unassign

    ; Also if new job IS craftsdwarf, don't unassign (job stays same effectively)
    cmp     dl, JOB_CRAFTSDWARF
    je      .no_craft_unassign

    ; Scan craft slots and decrement assigned count if > 0
    push    rdi
    push    rsi
    push    rcx
    push    rdx
    push    r8
    lea     r8, [rdi + GS_CRAFT]
    mov     ecx, RECIPE_COUNT
.craft_scan:
    test    ecx, ecx
    jz      .craft_scan_done
    movzx   eax, byte [r8 + CRAFT_ASSIGNED]
    test    eax, eax
    jz      .craft_scan_next
    dec     eax
    mov     byte [r8 + CRAFT_ASSIGNED], al
    ; if now 0, deactivate the slot
    test    eax, eax
    jnz     .craft_scan_next
    mov     byte [r8 + CRAFT_ACTIVE], 0
    mov     word [r8 + CRAFT_TIMER],  0
    jmp     .craft_scan_done    ; only remove from one slot per dwarf
.craft_scan_next:
    add     r8, SIZEOF_CRAFTSLOT
    dec     ecx
    jmp     .craft_scan
.craft_scan_done:
    pop     r8
    pop     rdx
    pop     rcx
    pop     rsi
    pop     rdi

.no_craft_unassign:
    mov     [rcx + DWARF_JOB],           dl
    mov     byte [rcx + DWARF_PREV_JOB], JOB_IDLE
    mov     rax, 1
    ret

.fail:
    xor     rax, rax
    ret