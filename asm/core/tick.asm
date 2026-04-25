; =========================================================
; asm/core/tick.asm - master tick dispatcher
; =========================================================

%include "core/offsets.inc"

extern asm_tick_resources
extern asm_tick_dwarves
extern asm_tick_events
extern asm_event_push

section .text
    global asm_tick

asm_tick:
    push    rbp
    mov     rbp, rsp
    push    rbx
    sub     rsp, 8

    mov     rbx, rdi

    ; 1. Advance tick counter
    inc     qword [rbx + GS_TICK]

    ; 2. Dwarf morale & fatigue
    mov     rdi, rbx
    call    asm_tick_dwarves

    ; 3. Flush pending dwarf event if any
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
    ; 4. Resource accumulation
    mov     rdi, rbx
    call    asm_tick_resources

    ; 5. Flavour events
    mov     rdi, rbx
    call    asm_tick_events

    add     rsp, 8
    pop     rbx
    pop     rbp
    ret