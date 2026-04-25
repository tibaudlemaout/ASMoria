; =========================================================
; asm/core/tick.asm - master tick dispatcher
; =========================================================

%include "core/offsets.inc"

extern asm_tick_resources
extern asm_tick_events

section .text
    global asm_tick

asm_tick:
    push    rbp
    mov     rbp, rsp
    push    rbx

    mov     rbx, rdi

    ; 1. Advance tick counter
    inc     qword [rbx + GS_TICK]

    ; 2. Resource accumulation
    mov     rdi, rbx
    call    asm_tick_resources

    ; 3. Event engine
    mov     rdi, rbx
    call    asm_tick_events

    pop     rbx
    pop     rbp
    ret