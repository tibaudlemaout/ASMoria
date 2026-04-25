; =========================================================
; asm/core/events.asm - Event engine
;
; Responsibilities:
;   1. asm_event_push  - write an EventRecord into the ring buffer
;   2. asm_tick_events - called each tick; fires a random flavour
;                        event every EVENT_INTERVAL ticks, picking
;                        a random alive dwarf and a random message
;                        code from the flavour pool.
;
; Message codes (interpreted by C ui/events_text.c):
;   Flavour pool  : 0x00 - 0x1F  (32 slots)
;   Positive pool : 0x20 - 0x2F  (16 slots)
;   Negative pool : 0x30 - 0x3F  (16 slots)
;   Milestone     : 0x40 - 0x4F  (16 slots)
;
; Exports:
;   asm_event_push(GameState*, code, severity, dwarf_idx)
;     rdi = state, rsi = code (uint8), rdx = severity, rcx = dwarf_idx
;
;   asm_tick_events(GameState*)
;     rdi = state
; =========================================================

%include "core/offsets.inc"

extern asm_rng_next

section .text
    global asm_event_push
    global asm_tick_events

; ---------------------------------------------------------
; asm_event_push(rdi=state, rsi=code, rdx=severity, rcx=dwarf_idx)
;
; Writes one EventRecord at event_log.entries[head], then
; advances head (mod EVENT_LOG_SIZE) and bumps count.
; ---------------------------------------------------------
asm_event_push:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14

    mov     r12, rdi                        ; r12 = state
    movzx   r13, sil                        ; r13 = code
    movzx   r14, dl                         ; r14 = severity
    ; rcx already = dwarf_idx (we'll use cl)

    ; base of event_log
    lea     rbx, [r12 + GS_EVENTLOG]

    ; slot = &entries[head]
    movzx   eax, byte [rbx + EVTLOG_HEAD]
    imul    eax, eax, SIZEOF_EVENTRECORD
    lea     rax, [rbx + EVTLOG_ENTRIES + rax]

    ; write EventRecord fields
    mov     r8, [r12 + GS_TICK]
    mov     [rax + EVT_TICK],      r8       ; tick
    mov     [rax + EVT_CODE],      r13b     ; code
    mov     [rax + EVT_SEVERITY],  r14b     ; severity
    mov     [rax + EVT_DWARF_IDX], cl       ; dwarf_idx

    ; advance head = (head + 1) % EVENT_LOG_SIZE
    movzx   edx, byte [rbx + EVTLOG_HEAD]
    inc     edx
    cmp     edx, EVENT_LOG_SIZE
    jl      .store_head
    xor     edx, edx
.store_head:
    mov     [rbx + EVTLOG_HEAD], dl

    ; bump count (capped at EVENT_LOG_SIZE)
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
; Fires a random flavour event every EVENT_INTERVAL ticks.
; Picks a random alive dwarf, picks a random code from the
; flavour pool (0x00-0x1F), pushes the record.
;
; Register map:
;   rbx = state
;   r12 = current tick
;   r13 = rng result scratch
;   r14 = chosen dwarf index
; ---------------------------------------------------------
asm_tick_events:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rbx, rdi

    ; only fire every EVENT_INTERVAL ticks
    mov     r12, [rbx + GS_TICK]
    mov     rax, r12
    xor     rdx, rdx
    mov     rcx, EVENT_INTERVAL
    div     rcx
    test    rdx, rdx                        ; rdx = tick % EVENT_INTERVAL
    jnz     .no_event

    ; --- pick a random alive dwarf ---
    ; roll RNG to get a candidate index
    mov     rdi, rbx
    call    asm_rng_next                    ; rax = random64
    xor     rdx, rdx
    mov     rcx, MAX_DWARVES
    div     rcx                             ; rdx = rax % MAX_DWARVES
    mov     r14, rdx                        ; r14 = candidate dwarf index

    ; check if alive; if not, scan forward for one that is
    mov     r15, MAX_DWARVES                ; loop budget
.find_alive:
    test    r15, r15
    jz      .no_event                       ; no alive dwarves at all

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
    ; --- pick a random flavour message code (0x00 - 0x1F) ---
    mov     rdi, rbx
    call    asm_rng_next
    and     rax, 0x1F                       ; rax = rax & 31

    ; --- push the event ---
    ; asm_event_push(state, code, EVT_FLAVOUR, dwarf_idx)
    mov     rdi, rbx
    mov     rsi, rax                        ; code
    mov     rdx, EVT_FLAVOUR                ; severity
    mov     rcx, r14                        ; dwarf_idx
    call    asm_event_push

.no_event:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret