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

extern asm_rng_next

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

    ; gold cost = base - discount + alive*5
    mov     r13, rax
    imul    r13, HIRE_GOLD_DISCOUNT
    mov     r14, HIRE_GOLD_BASE
    sub     r14, r13                    ; r14 = base gold cost after discount

    ; food cost — recompute recruiter level
    mov     rax, [rbx + GS_UPGR_TIER1]
    shr     rax, (UPGR_RECRUITERS * 4)
    and     rax, 0xF
    imul    rax, HIRE_FOOD_DISCOUNT
    mov     r13, HIRE_FOOD_BASE
    sub     r13, rax                    ; r13 = base food cost after discount

    ; count alive dwarves for surcharge (quick pre-scan)
    lea     rax, [rbx + GS_DWARVES]
    xor     r9d, r9d
    mov     rcx, MAX_DWARVES
.pre_count:
    test    rcx, rcx
    jz      .pre_count_done
    movzx   edx, byte [rax + DWARF_ALIVE]
    add     r9d, edx
    add     rax, SIZEOF_DWARF
    dec     rcx
    jmp     .pre_count
.pre_count_done:
    ; r9 = alive count — add surcharge
    mov     rax, r9
    imul    rax, 5
    add     r14, rax                    ; gold += alive * 5
    mov     rax, r9
    imul    rax, 2
    add     r13, rax                    ; food += alive * 2

    ; prestige hire discount
    mov     rax, [rbx + GS_PRESTIGE + PRESTIGE_NODES]
    bt      rax, PNODE_HIRE_25
    jnc     .check_hire_10
    ; -25%: multiply by 75/100
    imul    r14, 75
    xor     rdx, rdx
    mov     rax, r14
    mov     rcx, 100
    div     rcx
    mov     r14, rax
    imul    r13, 75
    mov     rax, r13
    xor     rdx, rdx
    div     rcx
    mov     r13, rax
    jmp     .hire_discount_done
.check_hire_10:
    bt      rax, PNODE_HIRE_10
    jnc     .hire_discount_done
    ; -10%: multiply by 90/100
    imul    r14, 90
    xor     rdx, rdx
    mov     rax, r14
    mov     rcx, 100
    div     rcx
    mov     r14, rax
    imul    r13, 90
    mov     rax, r13
    xor     rdx, rdx
    div     rcx
    mov     r13, rax
.hire_discount_done:

    cmp     r14, 10
    jge     .gold_ok
    mov     r14, 10
.gold_ok:
    cmp     r13, 5
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

    ; zero job_level[7]
    mov     dword [r12 + DWARF_JOB_LEVEL],     0
    mov     dword [r12 + DWARF_JOB_LEVEL + 3], 0   ; overlaps 1 byte, harmless (zeroed anyway)

    ; zero job_xp[7] (56 bytes)
    xor     rax, rax
    mov     [r12 + DWARF_JOB_XP +  0], rax
    mov     [r12 + DWARF_JOB_XP +  8], rax
    mov     [r12 + DWARF_JOB_XP + 16], rax
    mov     [r12 + DWARF_JOB_XP + 24], rax
    mov     [r12 + DWARF_JOB_XP + 32], rax
    mov     [r12 + DWARF_JOB_XP + 40], rax
    mov     [r12 + DWARF_JOB_XP + 48], rax

    ; pick a name: roll RNG, pick unused index
    ; try up to 64 times for a unique name
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8                      ; align for call

    ; r12 already = dwarf ptr (save before clobbering)
    ; we need to reload after these pushes since r12 was saved
    ; Actually r12 is still our dwarf ptr — just call rng
    mov     rdi, rbx
    call    asm_rng_next
    and     rax, 63                     ; 0-63

    ; check if any alive dwarf already has this name
    ; simple: iterate all alive dwarves, if clash increment and retry
    mov     r15, 64                     ; max attempts
.name_try:
    test    r15, r15
    jz      .name_done                  ; give up, use whatever

    lea     rcx, [rbx + GS_DWARVES]
    mov     r14, MAX_DWARVES
.name_scan:
    test    r14, r14
    jz      .name_done                  ; no clash found

    movzx   r8d, byte [rcx + DWARF_ALIVE]
    test    r8b, r8b
    jz      .name_scan_next

    movzx   r8d, byte [rcx + DWARF_NAME_IDX]
    cmp     r8b, al
    jne     .name_scan_next

    ; clash — try next index
    inc     al
    and     al, 63
    dec     r15
    jmp     .name_try

.name_scan_next:
    add     rcx, SIZEOF_DWARF
    dec     r14
    jmp     .name_scan

.name_done:
    ; restore dwarf ptr (r12 may have been pushed — reload it)
    imul    r9, rdx, SIZEOF_DWARF
    lea     r12, [rbx + GS_DWARVES + r9]
    mov     [r12 + DWARF_NAME_IDX], al

    ; --- hero trait roll (5% chance: rng % 20 == 0) ---
    ; Zero both hero fields first (default non-hero for all hires).
    mov     byte [r12 + DWARF_IS_HERO],    0
    mov     byte [r12 + DWARF_HERO_TRAIT], 0
    ; Use the alignment pad [rsp] to preserve rdx (slot_index) across
    ; the RNG calls without disturbing stack alignment (RSP is currently
    ; 16-aligned, so calls are safe without any extra pushes).
    mov     [rsp], rdx
    mov     rdi, rbx
    call    asm_rng_next            ; rax = random value
    mov     ecx, 20
    xor     edx, edx
    div     ecx                     ; rdx = rax % 20
    test    edx, edx
    jnz     .no_hero                ; not 0 → not a hero
    ; is a hero! pick trait 1..TRAIT_COUNT
    mov     rdi, rbx
    call    asm_rng_next
    mov     ecx, TRAIT_COUNT
    xor     edx, edx
    div     ecx                     ; rdx = rax % TRAIT_COUNT (0..5)
    inc     edx                     ; rdx = 1..TRAIT_COUNT (1..6)
    mov     byte [r12 + DWARF_IS_HERO],    1
    mov     byte [r12 + DWARF_HERO_TRAIT], dl
.no_hero:
    mov     rdx, [rsp]              ; restore slot_index

    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12

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