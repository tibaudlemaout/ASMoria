; =========================================================
; asm/core/resources.asm - per-tick resource accumulation
;
; Yield scaled by morale:
;   80-100 -> full
;   50-79  -> 75%
;   20-49  -> 50%
;    0-19  -> 25%
; =========================================================

%include "core/offsets.inc"

section .text
    global asm_tick_resources

; ---------------------------------------------------------
; apply_morale_scale(yield=rax, morale=cl) -> rax
; Local call — no stack frame needed, no further calls.
; Stack alignment: caller adds sub rsp,8 so we're safe.
; ---------------------------------------------------------
apply_morale_scale:
    cmp     cl, 80
    jge     .full
    cmp     cl, 50
    jge     .three_quarters
    cmp     cl, 20
    jge     .half
    shr     rax, 2          ; 25%
    ret
.half:
    shr     rax, 1          ; 50%
    ret
.three_quarters:
    lea     rax, [rax + rax*2]
    shr     rax, 2          ; 75%
    ret
.full:
    ret

; ---------------------------------------------------------
; asm_tick_resources(GameState *state [rdi])
;
; Stack accounting:
;   return addr : 8   (RSP 8 mod 16 on entry)
;   push rbp    : 16  aligned
;   push rbx    : 24
;   push r12    : 32  aligned
;   push r13    : 40
;   sub  rsp, 8 : 48  aligned  <- calls to apply_morale_scale happen here
; ---------------------------------------------------------
asm_tick_resources:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    sub     rsp, 8              ; realign for calls

    mov     r12, rdi
    lea     rsi, [rdi + GS_DWARVES]
    mov     rbx, MAX_DWARVES    ; loop counter (use rbx, free rcx for morale)

.loop:
    test    rbx, rbx
    jz      .done

    movzx   eax, byte [rsi + DWARF_ALIVE]
    test    al, al
    jz      .next

    movzx   eax, byte [rsi + DWARF_JOB]
    test    eax, eax
    jz      .next               ; idle: no production

    movzx   r13d, byte [rsi + DWARF_MORALE]

    cmp     al, JOB_MINER
    je      .do_miner
    cmp     al, JOB_LUMBERER
    je      .do_lumberer
    cmp     al, JOB_FARMER
    je      .do_farmer
    jmp     .next

.do_miner:
    mov     rax, 2
    mov     cl, r13b
    call    apply_morale_scale
    add     [r12 + GS_RESOURCES + RES_STONE], rax

    mov     rax, 1
    mov     cl, r13b
    call    apply_morale_scale
    add     [r12 + GS_RESOURCES + RES_GOLD], rax
    jmp     .next

.do_lumberer:
    mov     rax, 3
    mov     cl, r13b
    call    apply_morale_scale
    add     [r12 + GS_RESOURCES + RES_WOOD], rax
    jmp     .next

.do_farmer:
    mov     rax, 2
    mov     cl, r13b
    call    apply_morale_scale
    add     [r12 + GS_RESOURCES + RES_FOOD], rax

.next:
    add     rsi, SIZEOF_DWARF
    dec     rbx
    jmp     .loop

.done:
    add     rsp, 8
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret