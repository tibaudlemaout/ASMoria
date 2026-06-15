; =========================================================
; asm/core/resources.asm - per-tick resource accumulation
;
; Yield = (base + upgrade_level + job_level) * morale_scale * plenty_scale
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  push rbx: 24  push r12: 32
;   push r13: 40  push r14: 48  sub rsp,8: 64  aligned
; =========================================================

%include "core/offsets.inc"

section .text
    global asm_tick_resources

; ---------------------------------------------------------
; apply_morale_scale(yield=rax, morale=cl) -> rax
; Minimum yield of 1 if input > 0.
; ---------------------------------------------------------
apply_morale_scale:
    test    rax, rax
    jz      .zero
    cmp     cl, 80
    jge     .full
    cmp     cl, 50
    jge     .three_quarters
    cmp     cl, 20
    jge     .half
    shr     rax, 2
    jnz     .done
    mov     rax, 1
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
; apply_plenty(yield=rax, tier2_ptr=rdi_unused) -> rax
; Uses [rsp+8] as tier2 cache (set by caller before loop)
; yield * (100 + plenty_stacks*5) / 100
; Clobbers: rcx, rdx
; ---------------------------------------------------------
apply_plenty:
    ; r15 = tier2 (set by caller: mov r15, [rsp] then call)
    push    rdx
    mov     rcx, r15
    shr     rcx, (RUNE_PLENTY * 4)
    and     rcx, 0xF                    ; plenty stacks
    test    rcx, rcx
    jz      .no_plenty
    imul    rcx, 5
    add     rcx, 100                    ; rcx = 100 + stacks*5
    imul    rax, rcx                    ; rax = yield * multiplier
    xor     rdx, rdx
    mov     rcx, 100
    div     rcx                         ; rax = result
.no_plenty:
    pop     rdx
    ret

; ---------------------------------------------------------
; apply_depth_yield_inline
; r12=state, rax=yield -> rax = yield * (100 + (depth-1)*20) / 100
; depth 1=100%, 2=120%, 3=140%, 4=160%, 5=180%
; Clobbers: rcx, rdx
; ---------------------------------------------------------
apply_depth_yield_inline:
    push    rcx
    push    rdx
    mov     ecx, [r12 + GS_DEPTH]
    cmp     ecx, 2
    jl      .no_depth_bonus             ; depth 1 = no change
    dec     ecx                         ; depth-1
    imul    ecx, ecx, 20                ; (depth-1)*20
    add     ecx, 100                    ; 100 + bonus%
    movsx   rcx, ecx                    ; sign-extend to 64-bit
    imul    rax, rcx                    ; yield * multiplier
    xor     rdx, rdx
    mov     rcx, 100
    div     rcx                         ; / 100
.no_depth_bonus:
    pop     rdx
    pop     rcx
    ret

; ---------------------------------------------------------
; apply_prestige_yield(yield=rax, prestige_nodes=r14_saved) -> rax
; +2% per YIELD node unlocked (nodes in [r12 + GS_PRESTIGE + PRESTIGE_NODES])
; ---------------------------------------------------------
apply_prestige_yield:
    push    rcx
    push    rdx
    mov     rcx, [r12 + GS_PRESTIGE + PRESTIGE_NODES]
    ; count YIELD nodes (bits 0,1,2)
    xor     edx, edx
    bt      rcx, PNODE_YIELD_1
    adc     edx, 0
    bt      rcx, PNODE_YIELD_2
    adc     edx, 0
    bt      rcx, PNODE_YIELD_3
    adc     edx, 0
    test    edx, edx
    jz      .no_prestige_yield
    ; yield * (100 + stacks*2) / 100
    imul    edx, 2
    add     edx, 100
    imul    rax, rdx
    xor     rdx, rdx
    mov     rcx, 100
    div     rcx
.no_prestige_yield:
    pop     rdx
    pop     rcx
    ret

; ---------------------------------------------------------
; GET_UPGR_LEVEL macro: extracts nibble from r14 (tier1)
; result in rax, clobbers rcx
; ---------------------------------------------------------
%macro GET_UPGR_LEVEL 1
    mov     rax, r14
    mov     rcx, %1 * 4
    shr     rax, cl
    and     rax, 0xF
%endmacro

asm_tick_resources:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     r12, rdi
    lea     rsi, [rdi + GS_DWARVES]
    mov     rbx, MAX_DWARVES

    ; cache tier2 in r15 for plenty rune calculations
    mov     r15, [r12 + GS_UPGR_TIER2]

.loop:
    test    rbx, rbx
    jz      .done

    movzx   eax, byte [rsi + DWARF_ALIVE]
    test    al, al
    jz      .next

    movzx   eax, byte [rsi + DWARF_JOB]
    test    eax, eax
    jz      .next

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
    ; stone: 1 + pick_level + miner_job_level
    GET_UPGR_LEVEL UPGR_PICK_QUALITY
    movzx   rcx, byte [rsi + DWARF_JOB_LEVEL + JOB_MINER]
    add     rax, rcx
    add     rax, 1
    mov     cl, r13b
    call    apply_morale_scale
    call    apply_plenty
    call    apply_prestige_yield
    call    apply_depth_yield_inline
    add     [r12 + GS_RESOURCES + RES_STONE], rax
    add     [r12 + GS_PRESTIGE + PRESTIGE_RESOURCES], rax

    ; gold: 1 + pick_level + miner_job_level
    GET_UPGR_LEVEL UPGR_PICK_QUALITY
    movzx   rcx, byte [rsi + DWARF_JOB_LEVEL + JOB_MINER]
    add     rax, rcx
    add     rax, 1
    mov     cl, r13b
    call    apply_morale_scale
    call    apply_plenty
    call    apply_prestige_yield
    call    apply_depth_yield_inline
    add     [r12 + GS_RESOURCES + RES_GOLD], rax
    add     [r12 + GS_PRESTIGE + PRESTIGE_RESOURCES], rax
    jmp     .next

.do_lumberer:
    ; wood: 3 + saw_level + lumberer_job_level
    GET_UPGR_LEVEL UPGR_SAW_QUALITY
    movzx   rcx, byte [rsi + DWARF_JOB_LEVEL + JOB_LUMBERER]
    add     rax, rcx
    add     rax, 3
    mov     cl, r13b
    call    apply_morale_scale
    call    apply_plenty
    call    apply_prestige_yield
    call    apply_depth_yield_inline
    add     [r12 + GS_RESOURCES + RES_WOOD], rax
    add     [r12 + GS_PRESTIGE + PRESTIGE_RESOURCES], rax
    jmp     .next

.do_farmer:
    ; food: 2 + irr_level + farmer_job_level
    GET_UPGR_LEVEL UPGR_IRRIGATION
    movzx   rcx, byte [rsi + DWARF_JOB_LEVEL + JOB_FARMER]
    add     rax, rcx
    add     rax, 2
    mov     cl, r13b
    call    apply_morale_scale
    call    apply_plenty
    call    apply_prestige_yield
    call    apply_depth_yield_inline
    add     [r12 + GS_RESOURCES + RES_FOOD], rax
    add     [r12 + GS_PRESTIGE + PRESTIGE_RESOURCES], rax

.next:
    add     rsi, SIZEOF_DWARF
    dec     rbx
    jmp     .loop

.done:
    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret