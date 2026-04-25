; =========================================================
; asm/core/resources.asm - per-tick resource accumulation
;
; Called once per game tick. Iterates all dwarves and
; accumulates resources based on their assigned job.
;
; Exports:
;   asm_tick_resources(GameState *state)  [rdi]
; =========================================================

%include "core/offsets.inc"

section .text
    global asm_tick_resources

; ---------------------------------------------------------
; asm_tick_resources(GameState *state [rdi])
;
; Register map:
;   rdi = GameState*  (preserved)
;   rsi = &dwarves[i] (iterator)
;   rcx = loop counter (MAX_DWARVES down to 0)
;   al  = scratch for job/alive byte reads
; ---------------------------------------------------------
asm_tick_resources:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13

    mov     r12, rdi                        ; save state ptr
    lea     rsi, [rdi + GS_DWARVES]         ; rsi -> dwarves[0]
    mov     rcx, MAX_DWARVES

.loop:
    ; skip dead dwarves
    movzx   eax, byte [rsi + DWARF_ALIVE]
    test    al, al
    jz      .next

    ; dispatch on job
    movzx   eax, byte [rsi + DWARF_JOB]

    cmp     al, JOB_MINER
    je      .do_miner

    cmp     al, JOB_LUMBERER
    je      .do_lumberer

    cmp     al, JOB_FARMER
    je      .do_farmer

    jmp     .next                           ; JOB_IDLE / others: no yield

.do_miner:
    ; +2 stone, +1 gold per tick
    add     qword [r12 + GS_RESOURCES + RES_STONE], 2
    add     qword [r12 + GS_RESOURCES + RES_GOLD],  1
    jmp     .next

.do_lumberer:
    ; +3 wood per tick
    add     qword [r12 + GS_RESOURCES + RES_WOOD],  3
    jmp     .next

.do_farmer:
    ; +2 food per tick
    add     qword [r12 + GS_RESOURCES + RES_FOOD],  2
    jmp     .next

.next:
    add     rsi, SIZEOF_DWARF
    dec     rcx
    jnz     .loop

    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret