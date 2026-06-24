; =========================================================
; caravan.asm  --  Caravan trade and blueprint system
;
; All game logic (validation, timer, outcome resolution,
; blueprint rolls, penalty application) is in assembly.
; The C side (ui_caravan.c) reads state->raid.caravan_state
; directly via the CAVAN_* unpack macros in asmoria.h.
;
; State packing (uint32 at GS_RAID + RAID_CARAVAN_STATE):
;   bits  [1:0]   phase       CAVAN_IDLE/INFLIGHT/SUCCESS/FAIL
;   bits  [5:2]   workers-1   4 bits → 1-16 workers
;   bits  [8:6]   guards      3 bits → 0-7 guards
;   bits [15:9]   timer       7 bits → 0-127 ticks
;   bits [20:16]  food/100-1  5 bits → 0-31 (= 100-3200 food)
;   bit  [21]     bp_found    1 if a blueprint was obtained
;   bits [23:22]  bp_idx      wonder index (0-2)
; =========================================================
bits 64
%include "core/offsets.inc"

section .text
extern asm_rng_next

; =========================================================
; asm_caravan_launch(GameState *rdi, int workers esi,
;                   int guards edx, int64_t food rcx)
; Validates, deducts food, and packs the initial state.
; =========================================================
global asm_caravan_launch
asm_caravan_launch:
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rbx, rdi            ; state ptr
    mov     r12d, esi           ; workers (int)
    mov     r13d, edx           ; guards  (int)
    mov     r14, rcx            ; food    (int64_t)

    ; phase must be IDLE
    mov     eax, [rbx + GS_RAID + RAID_CARAVAN_STATE]
    and     eax, 0x3
    jnz     .done               ; not CAVAN_IDLE -> bail

    ; workers >= 1
    cmp     r12d, 1
    jl      .done

    ; food > 0
    test    r14, r14
    jle     .done

    ; have enough food
    mov     rax, [rbx + GS_RESOURCES + RES_FOOD]
    cmp     rax, r14
    jl      .done

    ; deduct food
    sub     [rbx + GS_RESOURCES + RES_FOOD], r14

    ; --- build packed state ---
    mov     r15d, CAVAN_INFLIGHT            ; bits [1:0]

    lea     ecx, [r12d - 1]                 ; workers - 1
    and     ecx, 0xF
    shl     ecx, 2
    or      r15d, ecx                        ; bits [5:2]

    mov     ecx, r13d                        ; guards
    and     ecx, 0x7
    shl     ecx, 6
    or      r15d, ecx                        ; bits [8:6]

    mov     ecx, CARAVAN_TRIP_TICKS         ; 60
    shl     ecx, 9
    or      r15d, ecx                        ; bits [15:9]

    ; food/100 - 1 -> bits [20:16]
    mov     rax, r14
    xor     rdx, rdx
    mov     rcx, CARAVAN_FOOD_STEP          ; 100
    div     rcx                              ; rax = food / 100
    dec     rax
    and     eax, 0x1F
    shl     eax, 16
    or      r15d, eax

    mov     [rbx + GS_RAID + RAID_CARAVAN_STATE], r15d

.done:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    ret

; =========================================================
; asm_tick_caravan(GameState *rdi)
;   Called from game_update every tick.
;   Decrements the timer; resolves outcome when it expires.
; =========================================================
global asm_tick_caravan
asm_tick_caravan:
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    ; 5 pushes (40 bytes) + return address (8) = 48 total -> 16-byte aligned stack

    mov     rbx, rdi

    ; phase must be INFLIGHT
    mov     r12d, [rbx + GS_RAID + RAID_CARAVAN_STATE]
    mov     eax,  r12d
    and     eax,  0x3
    cmp     eax,  CAVAN_INFLIGHT
    jne     .done

    ; extract timer from bits [15:9]
    mov     eax, r12d
    shr     eax, 9
    and     eax, 0x7F
    dec     eax
    jz      .resolve

    ; timer still > 0: write updated timer and return
    mov     ecx, r12d
    and     ecx, 0xFFFF01FF         ; clear bits [15:9]
    shl     eax, 9
    or      ecx, eax                ; reinsert decremented timer
    mov     [rbx + GS_RAID + RAID_CARAVAN_STATE], ecx
    jmp     .done

.resolve:
    ; Build result base: keep workers/guards/food, zero phase+timer+bp bits
    mov     r15d, r12d
    and     r15d, 0x001F01FC        ; keep bits [20:16],[8:6],[5:2]

    ; --- Roll for failure ---
    ; fail_pct = max(5,  60 - guards * 10)
    mov     eax, r12d
    shr     eax, 6
    and     eax, 0x7                ; guards
    imul    eax, 10
    neg     eax
    add     eax, 60                 ; 60 - guards*10
    cmp     eax, 5
    jge     .pct_ok
    mov     eax, 5
.pct_ok:
    mov     r13d, eax               ; r13d = fail_pct (callee-saved -> survives call)

    mov     rdi, rbx
    call    asm_rng_next            ; rax = random;  rbx,r12-r15 preserved
    xor     edx, edx
    mov     ecx, 100
    div     ecx                     ; edx = rax % 100

    cmp     edx, r13d
    jl      .fail_outcome

    ; =========================================================
    ; SUCCESS: roll for each un-blueprinted wonder
    ; =========================================================
    or      r15d, CAVAN_SUCCESS

    ; bonus = food/100  (stored as food/100-1 in bits [20:16])
    mov     eax, r12d
    shr     eax, 16
    and     eax, 0x1F
    inc     eax                     ; recover food/100
    mov     r13d, eax               ; r13d = blueprint bonus

    mov     r14, [rbx + GS_UPGR_TIER2]

    xor     r12d, r12d              ; wonder index w = 0

.bp_loop:
    cmp     r12d, WONDER_COUNT
    jge     .write_state

    ; skip if blueprint already owned
    movzx   rcx, r12b
    add     rcx, WONDER_BP_BIT_BASE
    bt      r14, rcx
    jc      .bp_next

    ; chance = (30 - w*10) + bonus, capped at 95
    mov     eax, r12d
    imul    eax, 10
    neg     eax
    add     eax, 30                 ; 30 / 20 / 10 for w = 0 / 1 / 2
    add     eax, r13d
    cmp     eax, 95
    jle     .chance_ok
    mov     eax, 95
.chance_ok:
    ; push chance (-8), then CALL (-8) = -16 total -> callee stack aligned
    push    rax
    mov     rdi, rbx
    call    asm_rng_next            ; rbx, r12-r15 preserved by callee
    pop     r9                      ; r9d = chance (recovered)
    xor     edx, edx
    mov     ecx, 100
    div     ecx                     ; edx = roll (0-99)

    cmp     edx, r9d
    jge     .bp_next

    ; --- Blueprint acquired for wonder r12d ---
    movzx   rcx, r12b
    add     rcx, WONDER_BP_BIT_BASE
    bts     r14, rcx                ; set blueprint bit in tier2
    mov     [rbx + GS_UPGR_TIER2], r14

    or      r15d, 0x200000          ; bit 21: bp_found = 1
    mov     eax, r12d
    shl     eax, 22
    or      r15d, eax               ; bits [23:22]: bp_idx

    jmp     .write_state            ; one blueprint per trip

.bp_next:
    inc     r12d
    jmp     .bp_loop

    ; =========================================================
    ; FAILURE: penalise (workers + guards) dwarves
    ; =========================================================
.fail_outcome:
    or      r15d, CAVAN_FAIL

    ; count = workers + guards  (r15d still has bits [5:2] and [8:6])
    mov     eax, r15d
    shr     eax, 2
    and     eax, 0xF
    inc     eax                     ; workers
    mov     ecx, r15d
    shr     ecx, 6
    and     ecx, 0x7
    add     eax, ecx                ; total = workers + guards
    mov     r13d, eax

    lea     rsi, [rbx + GS_DWARVES]
    xor     ecx, ecx                ; dwarf index i
    xor     edx, edx                ; applied count

.penalize:
    cmp     ecx, MAX_DWARVES
    jge     .write_state
    cmp     edx, r13d
    jge     .write_state
    cmp     byte [rsi + DWARF_ALIVE], 0
    je      .penalize_skip
    mov     byte [rsi + DWARF_FATIGUE], FATIGUE_MAX
    mov     byte [rsi + DWARF_MORALE], 0
    inc     edx
.penalize_skip:
    add     rsi, SIZEOF_DWARF
    inc     ecx
    jmp     .penalize

.write_state:
    mov     [rbx + GS_RAID + RAID_CARAVAN_STATE], r15d

.done:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    ret

; =========================================================
; asm_caravan_acknowledge(GameState *rdi)
;   Clears state when the player dismisses the result screen.
; =========================================================
global asm_caravan_acknowledge
asm_caravan_acknowledge:
    mov     eax, [rdi + GS_RAID + RAID_CARAVAN_STATE]
    and     eax, 0x3
    cmp     eax, CAVAN_SUCCESS
    je      .clear
    cmp     eax, CAVAN_FAIL
    je      .clear
    ret
.clear:
    mov     dword [rdi + GS_RAID + RAID_CARAVAN_STATE], 0
    ret