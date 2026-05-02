; =========================================================
; asm/core/events.asm - Event engine
;
; Two event rolls per tick window:
;   - Flavour event  every EVENT_INTERVAL ticks  (0x00-0x1F)
;   - Stat event     every STAT_INTERVAL  ticks  (0x20-0x3F)
;     Stat events call asm_apply_stat_event after logging.
; =========================================================

%include "core/offsets.inc"

extern asm_rng_next
extern asm_apply_stat_event

%define STAT_INTERVAL   80      ; ticks between stat-impact events

section .text
    global asm_event_push
    global asm_tick_events

; ---------------------------------------------------------
; asm_event_push(rdi=state, rsi=code, rdx=severity, rcx=dwarf_idx)
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  aligned
;   push rbx: 24
;   push r12: 32  aligned
;   push r13: 40
;   push r14: 48  aligned
;   5 pushes, NO calls — alignment irrelevant.
; ---------------------------------------------------------
asm_event_push:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14

    mov     r12, rdi
    movzx   r13, sil
    movzx   r14, dl

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
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  aligned
;   push rbx: 24
;   push r12: 32  aligned
;   push r13: 40
;   push r14: 48  aligned
;   push r15: 56
;   sub rsp,8: 64  aligned  <- all calls here
; ---------------------------------------------------------
asm_tick_events:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi

    ; -------------------------------------------------------
    ; Roll 1: flavour event every EVENT_INTERVAL ticks
    ; -------------------------------------------------------
    mov     rax, [rbx + GS_TICK]
    xor     rdx, rdx
    mov     rcx, EVENT_INTERVAL
    div     rcx
    test    rdx, rdx
    jnz     .check_stat

    call    .pick_alive_dwarf
    cmp     r14, -1
    je      .check_stat

    ; random flavour code 0x00-0x1F
    mov     rdi, rbx
    call    asm_rng_next
    and     rax, 0x1F

    mov     rdi, rbx
    mov     rsi, rax
    mov     rdx, EVT_FLAVOUR
    mov     rcx, r14
    call    asm_event_push

    ; -------------------------------------------------------
    ; Roll 2: stat-impact event every STAT_INTERVAL ticks
    ; -------------------------------------------------------
.check_stat:
    mov     rax, [rbx + GS_TICK]
    xor     rdx, rdx
    mov     rcx, STAT_INTERVAL
    div     rcx
    test    rdx, rdx
    jnz     .no_event

    call    .pick_alive_dwarf
    cmp     r14, -1
    je      .no_event

    ; random code: 50% positive (0x20-0x2F), 50% negative (0x30-0x3F)
    mov     rdi, rbx
    call    asm_rng_next
    mov     r15, rax                ; r15 = raw random

    ; bit 4 of random -> positive or negative pool
    mov     rax, r15
    shr     rax, 4
    and     rax, 1                  ; 0 = positive, 1 = negative

    mov     rcx, r15
    and     rcx, 0xF                ; low nibble = index within pool

    test    rax, rax
    jnz     .stat_negative

.stat_positive:
    add     rcx, 0x20               ; code 0x20-0x2F
    mov     r8, EVT_POSITIVE
    jmp     .fire_stat

.stat_negative:
    add     rcx, 0x30               ; code 0x30-0x3F
    mov     r8, EVT_NEGATIVE

.fire_stat:
    ; push event record
    mov     rdi, rbx
    mov     rsi, rcx                ; code
    mov     rdx, r8                 ; severity
    mov     rcx, r14                ; dwarf_idx
    call    asm_event_push

    ; apply stat change
    mov     rdi, rbx
    mov     rsi, [rsp + 8*0]        ; need code — save it first
    ; re-derive code from r15 since rcx was clobbered by asm_event_push
    mov     rax, r15
    shr     rax, 4
    and     rax, 1
    mov     rcx, r15
    and     rcx, 0xF
    test    rax, rax
    jnz     .apply_neg
    add     rcx, 0x20
    jmp     .apply_stat
.apply_neg:
    add     rcx, 0x30
.apply_stat:
    mov     rdi, rbx
    mov     rsi, rcx                ; code
    mov     rdx, r14                ; dwarf_idx
    call    asm_apply_stat_event

.no_event:
    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

; ---------------------------------------------------------
; .pick_alive_dwarf (local helper)
; Uses rbx=state. Returns dwarf index in r14, or -1.
; Clobbers: rax, rcx, rdx, r14, r15
; Must be called with aligned stack (sub rsp,8 already done).
; ---------------------------------------------------------
.pick_alive_dwarf:
    mov     rdi, rbx
    call    asm_rng_next
    xor     rdx, rdx
    mov     rcx, MAX_DWARVES
    div     rcx
    mov     r14, rdx

    mov     r15, MAX_DWARVES
.find_loop:
    test    r15, r15
    jz      .none_found

    lea     rax, [rbx + GS_DWARVES]
    imul    rcx, r14, SIZEOF_DWARF
    movzx   eax, byte [rax + rcx + DWARF_ALIVE]
    test    al, al
    jnz     .found

    inc     r14
    cmp     r14, MAX_DWARVES
    jl      .wrap
    xor     r14, r14
.wrap:
    dec     r15
    jmp     .find_loop

.none_found:
    mov     r14, -1
.found:
    ret