; =========================================================
; asm/core/tick.asm - master tick dispatcher
; =========================================================

%include "core/offsets.inc"

extern asm_tick_resources
extern asm_tick_dwarves
extern asm_tick_events
extern asm_tick_breach
extern asm_tick_xp
extern asm_event_push
extern asm_tick_infra
extern asm_tick_upkeep
extern asm_tick_depth

section .text
    global asm_tick

; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  aligned
;   push rbx: 24
;   sub rsp,8: 32  aligned
asm_tick:
    push    rbp
    mov     rbp, rsp
    push    rbx
    sub     rsp, 8

    mov     rbx, rdi

    inc     qword [rbx + GS_TICK]

    ; 1. Dwarf morale/fatigue
    mov     rdi, rbx
    call    asm_tick_dwarves

    ; 2. Flush pending event from dwarves
    movzx   eax, byte [rbx + GS_PENDING + PENDING_CODE]
    cmp     al, 0xFF
    je      .no_pending
    movzx   esi, byte [rbx + GS_PENDING + PENDING_CODE]
    movzx   edx, byte [rbx + GS_PENDING + PENDING_SEVERITY]
    movzx   ecx, byte [rbx + GS_PENDING + PENDING_DWARF]
    mov     rdi, rbx
    call    asm_event_push
    mov     byte [rbx + GS_PENDING + PENDING_CODE], 0xFF

.no_pending:
    ; 3. XP gain + level-up detection
    mov     rdi, rbx
    call    asm_tick_xp

    ; 4. Flush pending event from XP (level-up)
    movzx   eax, byte [rbx + GS_PENDING + PENDING_CODE]
    cmp     al, 0xFF
    je      .no_xp_pending
    movzx   esi, byte [rbx + GS_PENDING + PENDING_CODE]
    movzx   edx, byte [rbx + GS_PENDING + PENDING_SEVERITY]
    movzx   ecx, byte [rbx + GS_PENDING + PENDING_DWARF]
    mov     rdi, rbx
    call    asm_event_push
    mov     byte [rbx + GS_PENDING + PENDING_CODE], 0xFF

.no_xp_pending:
    ; 5. Resources (uses job levels for bonus yield)
    mov     rdi, rbx
    call    asm_tick_resources

    ; 6. Building upkeep (before infra so degraded buildings dont produce)
    mov     rdi, rbx
    call    asm_tick_upkeep

    ; 7. Depth resource yields
    mov     rdi, rbx
    call    asm_tick_depth

    ; 8. Infrastructure effects (Scholar/Mana Well) — skipped if degraded
    mov     rdi, rbx
    call    asm_tick_infra

    ; 8. Raid / Breach system
    mov     rdi, rbx
    call    asm_tick_breach

    ; 9. Flavour + stat events
    mov     rdi, rbx
    call    asm_tick_events

    add     rsp, 8
    pop     rbx
    pop     rbp
    ret