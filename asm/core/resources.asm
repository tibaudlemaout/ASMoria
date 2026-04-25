; =========================================================
; asm/core/resources.asm - per-tick resource accumulation
;
; Yield is scaled by dwarf morale:
;   morale 80-100 -> full yield
;   morale 50-79  -> 75% yield  (yield * 3 / 4)
;   morale 20-49  -> 50% yield  (yield / 2)
;   morale  0-19  -> 25% yield  (yield / 4)
;
; Exports:
;   asm_tick_resources(GameState *state)  [rdi]
; =========================================================

%include "core/offsets.inc"

section .text
    global asm_tick_resources

; ---------------------------------------------------------
; apply_morale_scale(yield [rax], morale [cl]) -> rax
; Scales yield by morale tier. Inline macro via call.
; ---------------------------------------------------------
apply_morale_scale:
    cmp     cl, 80
    jge     .full
    cmp     cl, 50
    jge     .three_quarters
    cmp     cl, 20
    jge     .half
    ; 0-19: quarter yield
    shr     rax, 2
    ret
.half:
    shr     rax, 1
    ret
.three_quarters:
    ; yield * 3 / 4
    lea     rax, [rax + rax*2]  ; rax = yield * 3
    shr     rax, 2
    ret
.full:
    ret                         ; rax unchanged

; ---------------------------------------------------------
; asm_tick_resources(GameState *state [rdi])
;
; Register map:
;   r12 = state ptr
;   rsi = &dwarves[i]
;   rcx = loop counter
;   r13 = base yield for current job
; ---------------------------------------------------------
asm_tick_resources:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13

    mov     r12, rdi
    lea     rsi, [rdi + GS_DWARVES]
    mov     rcx, MAX_DWARVES

.loop:
    movzx   eax, byte [rsi + DWARF_ALIVE]
    test    al, al
    jz      .next

    ; skip idle dwarves - no resource production
    movzx   eax, byte [rsi + DWARF_JOB]
    test    eax, eax
    jz      .next

    movzx   r13d, byte [rsi + DWARF_MORALE]  ; r13 = morale for scaling

    cmp     al, JOB_MINER
    je      .do_miner
    cmp     al, JOB_LUMBERER
    je      .do_lumberer
    cmp     al, JOB_FARMER
    je      .do_farmer
    jmp     .next

.do_miner:
    ; base: +2 stone
    mov     rax, 2
    mov     cl, r13b
    call    apply_morale_scale
    add     [r12 + GS_RESOURCES + RES_STONE], rax

    ; base: +1 gold
    mov     rax, 1
    mov     cl, r13b
    call    apply_morale_scale
    add     [r12 + GS_RESOURCES + RES_GOLD], rax
    jmp     .next

.do_lumberer:
    ; base: +3 wood
    mov     rax, 3
    mov     cl, r13b
    call    apply_morale_scale
    add     [r12 + GS_RESOURCES + RES_WOOD], rax
    jmp     .next

.do_farmer:
    ; base: +2 food
    mov     rax, 2
    mov     cl, r13b
    call    apply_morale_scale
    add     [r12 + GS_RESOURCES + RES_FOOD], rax
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