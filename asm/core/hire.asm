; =========================================================
; asm/core/hire.asm - Dwarf hiring
;
; Costs are reduced by Recruiters level:
;   gold = HIRE_GOLD_BASE - (recruiters_level * HIRE_GOLD_DISCOUNT)
;   food = HIRE_FOOD_BASE - (recruiters_level * HIRE_FOOD_DISCOUNT)
;   (minimum 10 gold, 5 food)
;
; Dwarf cap = DWARF_CAP_BASE + (barracks_level * DWARF_CAP_PER_LEVEL)
; Scan only up to cap, not full MAX_DWARVES.
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  aligned
;   push rbx: 24
;   push r12: 32  aligned
;   push r13: 40
;   push r14: 48  aligned
;   push r15: 56
;   6 pushes, NO calls — alignment irrelevant.
; =========================================================

%include "core/offsets.inc"

section .text
    global asm_hire_dwarf

asm_hire_dwarf:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rbx, rdi

    ; --- compute current dwarf cap ---
    ; barracks_level = (tier1 >> (UPGR_BARRACKS*4)) & 0xF
    mov     rax, [rbx + GS_UPGR_TIER1]
    shr     rax, (UPGR_BARRACKS * 4)
    and     rax, 0xF                    ; rax = barracks level
    imul    rax, DWARF_CAP_PER_LEVEL
    add     rax, DWARF_CAP_BASE         ; rax = cap
    mov     r15, rax                    ; r15 = dwarf cap

    ; --- compute hire costs with recruiter discount ---
    mov     rax, [rbx + GS_UPGR_TIER1]
    shr     rax, (UPGR_RECRUITERS * 4)
    and     rax, 0xF                    ; rax = recruiters level

    ; gold cost
    mov     r13, rax
    imul    r13, HIRE_GOLD_DISCOUNT
    mov     r14, HIRE_GOLD_BASE
    sub     r14, r13                    ; r14 = gold cost
    cmp     r14, 10                     ; floor at 10
    jge     .gold_ok
    mov     r14, 10
.gold_ok:

    ; food cost (recompute recruiter level from rax still valid? no, rax clobbered)
    mov     rax, [rbx + GS_UPGR_TIER1]
    shr     rax, (UPGR_RECRUITERS * 4)
    and     rax, 0xF
    imul    rax, HIRE_FOOD_DISCOUNT
    mov     r13, HIRE_FOOD_BASE
    sub     r13, rax                    ; r13 = food cost
    cmp     r13, 5                      ; floor at 5
    jge     .food_ok
    mov     r13, 5
.food_ok:

    ; --- check resources ---
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    cmp     [rbx + GS_RESOURCES + RES_FOOD], r13
    jl      .fail

    ; --- count alive dwarves and find free slot within cap ---
    lea     r12, [rbx + GS_DWARVES]
    xor     rcx, rcx                    ; rcx = slot index
    mov     rdx, -1                     ; rdx = first free slot (-1=none)
    xor     r8, r8                      ; r8  = alive count

.scan:
    cmp     rcx, r15                    ; stop at cap
    jge     .scan_done

    movzx   eax, byte [r12 + DWARF_ALIVE]
    test    al, al
    jnz     .count_alive

    ; free slot — record first one
    cmp     rdx, -1
    jne     .next_slot
    mov     rdx, rcx
    jmp     .next_slot

.count_alive:
    inc     r8

.next_slot:
    add     r12, SIZEOF_DWARF
    inc     rcx
    jmp     .scan

.scan_done:
    ; no free slot within cap?
    cmp     rdx, -1
    je      .fail

    ; --- deduct resources ---
    sub     [rbx + GS_RESOURCES + RES_GOLD], r14
    sub     [rbx + GS_RESOURCES + RES_FOOD], r13

    ; --- init dwarf at slot rdx ---
    imul    rax, rdx, SIZEOF_DWARF
    lea     r12, [rbx + GS_DWARVES + rax]

    mov     byte  [r12 + DWARF_ALIVE],    1
    mov     byte  [r12 + DWARF_JOB],      JOB_IDLE
    mov     byte  [r12 + DWARF_MORALE],   80
    mov     byte  [r12 + DWARF_FATIGUE],  0
    mov     byte  [r12 + DWARF_PREV_JOB], JOB_IDLE
    mov     qword [r12 + DWARF_XP],       0

    ; queue milestone event
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x42
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_MILESTONE
    mov     [rbx + GS_PENDING + PENDING_DWARF],         dl

    mov     rax, rdx                    ; return slot index
    jmp     .done

.fail:
    mov     rax, -1

.done:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret