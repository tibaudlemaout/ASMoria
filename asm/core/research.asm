; =========================================================
; asm/core/research.asm - Rune research system
;
; asm_buy_rune(state, rune_id) -> rax = new stack or 0
;
; Rune levels packed as 4-bit nibbles in GS_UPGR_TIER2.
; Cost is flat mana per stack (no gold/stone).
; Requires Rune Halls Lv5 to research anything.
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  push rbx: 24  push r12: 32
;   push r13: 40  push r14: 48  push r15: 56
;   6 pushes, NO calls.
; =========================================================

%include "core/offsets.inc"

section .data
; Mana costs indexed by rune id
rune_costs: dq RUNE_COST_ENDURANCE, RUNE_COST_PLENTY,  RUNE_COST_SWIFTNESS, \
               RUNE_COST_WARDING,   RUNE_COST_KINSHIP,  RUNE_COST_DEEP

; Max stacks indexed by rune id
rune_maxes: db RUNE_MAX_SMALL, RUNE_MAX_SMALL, RUNE_MAX_SMALL, \
               RUNE_MAX_LARGE, RUNE_MAX_LARGE, RUNE_MAX_LARGE

section .text
    global asm_buy_rune

; ---------------------------------------------------------
; asm_buy_rune(rdi=state, rsi=rune_id) -> rax
; ---------------------------------------------------------
asm_buy_rune:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rbx, rdi
    movzx   r12, sil                    ; r12 = rune id

    ; bounds check
    cmp     r12, RUNE_COUNT
    jge     .fail

    ; require Rune Halls Lv5
    mov     rax, [rbx + GS_UPGR_TIER1]
    shr     rax, (UPGR_RUNE_HALLS * 4)
    and     rax, 0xF
    cmp     rax, 5
    jl      .fail

    ; get current stack
    mov     rax, [rbx + GS_UPGR_TIER2]
    mov     rcx, r12
    shl     rcx, 2
    shr     rax, cl
    and     rax, 0xF
    mov     r13, rax                    ; r13 = current stacks

    ; check max
    lea     rax, [rel rune_maxes]
    movzx   r14, byte [rax + r12]       ; r14 = max stacks
    cmp     r13, r14
    jge     .fail

    ; get mana cost
    lea     rax, [rel rune_costs]
    mov     r15, [rax + r12 * 8]        ; r15 = mana cost

    ; check mana
    cmp     [rbx + GS_RESOURCES + RES_MANA], r15
    jl      .fail

    ; deduct mana
    sub     [rbx + GS_RESOURCES + RES_MANA], r15

    ; increment stack in tier2
    mov     r14, r13
    inc     r14                         ; new stack count

    mov     rcx, r12
    shl     rcx, 2
    mov     rax, 0xF
    shl     rax, cl
    not     rax
    and     [rbx + GS_UPGR_TIER2], rax  ; clear nibble
    mov     rax, r14
    shl     rax, cl
    or      [rbx + GS_UPGR_TIER2], rax  ; set nibble

    ; queue milestone event 0x44
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x44
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_MILESTONE
    mov     byte [rbx + GS_PENDING + PENDING_DWARF],    0xFF

    mov     rax, r14
    jmp     .done

.fail:
    xor     rax, rax

.done:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret