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

; STACK:
;   entry   :  8 mod 16
;   push rbp: 16         aligned
;   push rbx: 24
;   sub rsp,8: 32        aligned  <- all calls here
asm_tick:
    push    rbp
    mov     rbp, rsp
    push    rbx
    sub     rsp, 8

    mov     rbx, rdi

    inc     qword [rbx + GS_TICK]

    mov     rdi, rbx
    call    asm_tick_dwarves

    ; flush pending event from dwarves subsystem
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
    mov     rdi, rbx
    call    asm_tick_resources

    mov     rdi, rbx
    call    asm_tick_events

    add     rsp, 8
    pop     rbx
    pop     rbp
    ret