; =========================================================
; asm/core/events.asm - Event engine
; =========================================================

%include "core/offsets.inc"

extern asm_rng_next

section .text
    global asm_event_push
    global asm_tick_events

; ---------------------------------------------------------
; asm_event_push(rdi=state, rsi=code, rdx=severity, rcx=dwarf_idx)
;
; Stack on entry: return addr pushed = RSP is 8 mod 16
; push rbp  -> 16 mod 16  (aligned)
; push rbx  ->  8 mod 16
; push r12  -> 16 mod 16  (aligned)
; push r13  ->  8 mod 16
; push r14  -> 16 mod 16  (aligned)
; No calls inside, so no alignment needed here.
; ---------------------------------------------------------
asm_event_push:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14

    mov     r12, rdi
    movzx   r13, sil                        ; code
    movzx   r14, dl                         ; severity

    lea     rbx, [r12 + GS_EVENTLOG]

    movzx   eax, byte [rbx + EVTLOG_HEAD]
    imul    eax, eax, SIZEOF_EVENTRECORD
    lea     rax, [rbx + EVTLOG_ENTRIES + rax]

    mov     r8, [r12 + GS_TICK]
    mov     [rax + EVT_TICK],      r8
    mov     [rax + EVT_CODE],      r13b
    mov     [rax + EVT_SEVERITY],  r14b
    mov     [rax + EVT_DWARF_IDX], cl

    movzx   edx, byte [rbx + EVTLOG_HEAD]
    inc     edx
    cmp     edx, EVENT_LOG_SIZE
    jl      .store_head
    xor     edx, edx
.store_head:
    mov     [rbx + EVTLOG_HEAD], dl

    movzx   edx, byte [rbx + EVTLOG_COUNT]
    cmp     edx, EVENT_LOG_SIZE
    jge     .done
    inc     edx
    mov     [rbx + EVTLOG_COUNT], dl

.done:
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

; ---------------------------------------------------------
; asm_tick_events(rdi=state)
;
; Stack accounting:
;   return addr         :  8 bytes  (RSP 8 mod 16 on entry)
;   push rbp            : +8 = 16   aligned
;   push rbx            : +8 = 24
;   push r12            : +8 = 32   aligned
;   push r13            : +8 = 40
;   push r14            : +8 = 48   aligned
;   push r15            : +8 = 56
;   sub  rsp, 8         : +8 = 64   aligned  <- calls happen here
; ---------------------------------------------------------
asm_tick_events:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8              ; realign to 16 bytes for calls

    mov     rbx, rdi

    ; fire every EVENT_INTERVAL ticks
    mov     rax, [rbx + GS_TICK]
    xor     rdx, rdx
    mov     rcx, EVENT_INTERVAL
    div     rcx
    test    rdx, rdx
    jnz     .no_event

    ; pick random alive dwarf
    mov     rdi, rbx
    call    asm_rng_next
    xor     rdx, rdx
    mov     rcx, MAX_DWARVES
    div     rcx
    mov     r14, rdx            ; candidate index

    mov     r15, MAX_DWARVES    ; loop budget
.find_alive:
    test    r15, r15
    jz      .no_event

    lea     rax, [rbx + GS_DWARVES]
    imul    rcx, r14, SIZEOF_DWARF
    movzx   eax, byte [rax + rcx + DWARF_ALIVE]
    test    al, al
    jnz     .dwarf_found

    inc     r14
    cmp     r14, MAX_DWARVES
    jl      .wrap_ok
    xor     r14, r14
.wrap_ok:
    dec     r15
    jmp     .find_alive

.dwarf_found:
    ; pick random flavour code
    mov     rdi, rbx
    call    asm_rng_next
    and     rax, 0x1F

    ; push event
    mov     rdi, rbx
    mov     rsi, rax
    mov     rdx, EVT_FLAVOUR
    mov     rcx, r14
    call    asm_event_push

.no_event:
    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret