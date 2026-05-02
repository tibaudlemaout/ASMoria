; =========================================================
; asm/core/hire.asm - Dwarf hiring
;
; asm_hire_dwarf(GameState *state) -> int8_t
;   rdi = state
;   rax = index of hired dwarf (0-63), or -1 if failed
;
; Cost: HIRE_GOLD gold + HIRE_FOOD food
; Fails if: not enough resources, or no free slots
;
; On success:
;   - Deducts resources
;   - Initialises the dwarf slot (alive, idle, full morale)
;   - Pushes a milestone event
; =========================================================

%include "core/offsets.inc"

extern asm_event_push

%define HIRE_GOLD   50
%define HIRE_FOOD   20

section .text
    global asm_hire_dwarf

; ---------------------------------------------------------
; asm_hire_dwarf(rdi=state) -> rax = dwarf index or -1
; ---------------------------------------------------------
asm_hire_dwarf:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    sub     rsp, 8              ; align stack for call

    mov     rbx, rdi            ; rbx = state

    ; --- check gold ---
    mov     rax, [rbx + GS_RESOURCES + RES_GOLD]
    cmp     rax, HIRE_GOLD
    jl      .fail

    ; --- check food ---
    mov     rax, [rbx + GS_RESOURCES + RES_FOOD]
    cmp     rax, HIRE_FOOD
    jl      .fail

    ; --- find a free dwarf slot ---
    lea     r12, [rbx + GS_DWARVES]     ; r12 = &dwarves[0]
    xor     r13, r13                    ; r13 = slot index

.find_slot:
    cmp     r13, MAX_DWARVES
    jge     .fail                       ; no free slot

    movzx   eax, byte [r12 + DWARF_ALIVE]
    test    al, al
    jz      .found_slot

    add     r12, SIZEOF_DWARF
    inc     r13
    jmp     .find_slot

.found_slot:
    ; --- deduct resources ---
    sub     qword [rbx + GS_RESOURCES + RES_GOLD], HIRE_GOLD
    sub     qword [rbx + GS_RESOURCES + RES_FOOD], HIRE_FOOD

    ; --- initialise dwarf ---
    mov     byte [r12 + DWARF_ALIVE],    1
    mov     byte [r12 + DWARF_JOB],      JOB_IDLE
    mov     byte [r12 + DWARF_MORALE],   80
    mov     byte [r12 + DWARF_FATIGUE],  0
    mov     byte [r12 + DWARF_PREV_JOB], JOB_IDLE
    mov     byte [r12 + DWARF_XP],       0
    mov     qword [r12 + DWARF_XP],      0

    ; --- push milestone event 0x42 "a new dwarf joins" ---
    mov     rdi, rbx
    mov     rsi, 0x42
    mov     rdx, EVT_MILESTONE
    mov     rcx, r13
    call    asm_event_push

    ; --- return slot index ---
    mov     rax, r13
    jmp     .done

.fail:
    mov     rax, -1

.done:
    add     rsp, 8
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret