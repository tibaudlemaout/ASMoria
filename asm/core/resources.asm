; =========================================================
; asm/core/resources.asm - per-tick resource accumulation
;
; Yields per working dwarf (before morale scale):
;   Miner:    stone = 2 + pick_level
;             gold  = 1 + pick_level  (separate, not halved)
;   Lumberer: wood  = 3 + saw_level
;   Farmer:   food  = 2 + irr_level
;
; Morale scaling applied after upgrade bonus:
;   80-100 -> full
;   50-79  -> 75%  (minimum 1 if base > 0)
;   20-49  -> 50%  (minimum 1 if base > 0)
;   0-19   -> 25%  (minimum 1 if base > 0)
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  aligned
;   push rbx: 24
;   push r12: 32  aligned
;   push r13: 40
;   push r14: 48  aligned
;   sub rsp,8: 56 -> 64  aligned  <- calls here
;   5 pushes + sub rsp,8 -> aligned.
; =========================================================

%include "core/offsets.inc"

section .text
    global asm_tick_resources

; ---------------------------------------------------------
; apply_morale_scale(yield=rax, morale=cl) -> rax
; Guarantees minimum yield of 1 if input yield > 0.
; ---------------------------------------------------------
apply_morale_scale:
    test    rax, rax
    jz      .zero               ; 0 in -> 0 out
    cmp     cl, 80
    jge     .full
    cmp     cl, 50
    jge     .three_quarters
    cmp     cl, 20
    jge     .half
    ; 25%
    shr     rax, 2
    jnz     .done
    mov     rax, 1              ; floor at 1
    ret
.half:
    shr     rax, 1
    jnz     .done
    mov     rax, 1
    ret
.three_quarters:
    lea     rax, [rax + rax*2]
    shr     rax, 2
    jnz     .done
    mov     rax, 1
    ret
.full:
.done:
    ret
.zero:
    ret

; ---------------------------------------------------------
; get_pick_level() / get_saw_level() / get_irr_level()
; Inline helpers: extract nibble from r14 (tier1).
; Return value in rax. Clobbers rcx.
; ---------------------------------------------------------
%macro GET_UPGR_LEVEL 1          ; %1 = upgrade id (0,1,2)
    mov     rax, r14
    mov     rcx, %1 * 4
    shr     rax, cl
    and     rax, 0xF
%endmacro

; ---------------------------------------------------------
; asm_tick_resources(GameState *state [rdi])
; r12 = state, rsi = current dwarf ptr, rbx = loop counter
; r13 = morale, r14 = tier1
; ---------------------------------------------------------
asm_tick_resources:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    sub     rsp, 8

    mov     r12, rdi
    lea     rsi, [rdi + GS_DWARVES]
    mov     rbx, MAX_DWARVES

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
    mov     r14, [r12 + GS_UPGR_TIER1]

    cmp     al, JOB_MINER
    je      .do_miner
    cmp     al, JOB_LUMBERER
    je      .do_lumberer
    cmp     al, JOB_FARMER
    je      .do_farmer
    jmp     .next

.do_miner:
    ; stone: 2 + pick_level
    GET_UPGR_LEVEL UPGR_PICK_QUALITY
    add     rax, 2
    mov     cl, r13b
    call    apply_morale_scale
    add     [r12 + GS_RESOURCES + RES_STONE], rax

    ; gold: 1 + pick_level (same bonus, not halved)
    GET_UPGR_LEVEL UPGR_PICK_QUALITY
    add     rax, 1
    mov     cl, r13b
    call    apply_morale_scale
    add     [r12 + GS_RESOURCES + RES_GOLD], rax
    jmp     .next

.do_lumberer:
    ; wood: 3 + saw_level
    GET_UPGR_LEVEL UPGR_SAW_QUALITY
    add     rax, 3
    mov     cl, r13b
    call    apply_morale_scale
    add     [r12 + GS_RESOURCES + RES_WOOD], rax
    jmp     .next

.do_farmer:
    ; food: 2 + irr_level
    GET_UPGR_LEVEL UPGR_IRRIGATION
    add     rax, 2
    mov     cl, r13b
    call    apply_morale_scale
    add     [r12 + GS_RESOURCES + RES_FOOD], rax

.next:
    add     rsi, SIZEOF_DWARF
    dec     rbx
    jmp     .loop

.done:
    add     rsp, 8
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret