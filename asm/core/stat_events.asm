; =========================================================
; asm/core/stat_events.asm - Stat-impact event application
;
; asm_apply_stat_event(state, code, dwarf_idx)
;   rdi = state
;   rsi = event code
;   rdx = dwarf index (0-63)
;
; Positive events (0x20-0x2F): morale/xp gains
; Negative events (0x30-0x3F): morale/fatigue losses
;
; Stat changes are clamped to valid ranges.
; Does NOT push to the event log — caller does that.
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  aligned
;   push rbx: 24
;   push r12: 32  aligned
;   push r13: 40
;   Total: 4 pushes, NO calls — no sub rsp needed.
; =========================================================

%include "core/offsets.inc"

%define MORALE_MAX  100
%define FATIGUE_MAX 100
%define XP_PER_HIT  0           ; base, overridden per event

section .text
    global asm_apply_stat_event

asm_apply_stat_event:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13

    mov     rbx, rdi                    ; rbx = state
    movzx   r12, sil                    ; r12 = event code
    movzx   r13, dl                     ; r13 = dwarf index

    ; get dwarf ptr
    imul    rax, r13, SIZEOF_DWARF
    lea     r13, [rbx + GS_DWARVES + rax]  ; r13 = &dwarves[idx]

    ; sanity: dwarf must be alive
    movzx   eax, byte [r13 + DWARF_ALIVE]
    test    al, al
    jz      .done

    ; dispatch on code range
    cmp     r12b, 0x20
    jl      .done               ; 0x00-0x1F: flavour only, no stat change
    cmp     r12b, 0x30
    jl      .positive
    cmp     r12b, 0x40
    jl      .negative
    jmp     .done               ; 0x40+: milestone, no stat change

    ; =======================================================
    ; POSITIVE events
    ; =======================================================
.positive:
    cmp     r12b, 0x20          ; finds rich gold seam [+MORALE +8]
    je      .morale_p8
    cmp     r12b, 0x21          ; hard work pays off [+XP +50]
    je      .xp_p50
    cmp     r12b, 0x23          ; technique improves [+XP +30]
    je      .xp_p30
    cmp     r12b, 0x25          ; foreman praises [+MORALE +5]
    je      .morale_p5
    cmp     r12b, 0x27          ; natural spring [+MORALE +4]
    je      .morale_p4
    cmp     r12b, 0x2A          ; trains younger dwarf [+XP +20]
    je      .xp_p20
    cmp     r12b, 0x2B          ; finds ancient tool [+MORALE +5]
    je      .morale_p5
    cmp     r12b, 0x2E          ; solves drainage [+XP +25]
    je      .xp_p25
    cmp     r12b, 0x2F          ; finds ancient map [+MORALE +6]
    je      .morale_p6
    jmp     .done               ; other positive codes: no stat change

.morale_p8:
    movzx   eax, byte [r13 + DWARF_MORALE]
    add     eax, 8
    jmp     .clamp_morale
.morale_p6:
    movzx   eax, byte [r13 + DWARF_MORALE]
    add     eax, 6
    jmp     .clamp_morale
.morale_p5:
    movzx   eax, byte [r13 + DWARF_MORALE]
    add     eax, 5
    jmp     .clamp_morale
.morale_p4:
    movzx   eax, byte [r13 + DWARF_MORALE]
    add     eax, 4
.clamp_morale:
    cmp     eax, MORALE_MAX
    jle     .store_morale
    mov     eax, MORALE_MAX
.store_morale:
    mov     [r13 + DWARF_MORALE], al
    jmp     .done

.xp_p50:
    movzx   rcx, byte [r13 + DWARF_JOB]
    shl     rcx, 3
    add     qword [r13 + DWARF_JOB_XP + rcx], 50
    jmp     .done
.xp_p30:
    movzx   rcx, byte [r13 + DWARF_JOB]
    shl     rcx, 3
    add     qword [r13 + DWARF_JOB_XP + rcx], 30
    jmp     .done
.xp_p25:
    movzx   rcx, byte [r13 + DWARF_JOB]
    shl     rcx, 3
    add     qword [r13 + DWARF_JOB_XP + rcx], 25
    jmp     .done
.xp_p20:
    movzx   rcx, byte [r13 + DWARF_JOB]
    shl     rcx, 3
    add     qword [r13 + DWARF_JOB_XP + rcx], 20
    jmp     .done

    ; =======================================================
    ; NEGATIVE events
    ; =======================================================
.negative:
    cmp     r12b, 0x30          ; twists ankle [+FATIGUE +15]
    je      .fatigue_p15
    cmp     r12b, 0x31          ; breathes dust [+FATIGUE +10]
    je      .fatigue_p10
    cmp     r12b, 0x33          ; cave-in [-MORALE -8]
    je      .morale_m8
    cmp     r12b, 0x34          ; argues [-MORALE -5]
    je      .morale_m5
    cmp     r12b, 0x36          ; spooked [-MORALE -6]
    je      .morale_m6
    cmp     r12b, 0x39          ; tools pilfered [-MORALE -7]
    je      .morale_m7
    cmp     r12b, 0x3B          ; pick bounces [+FATIGUE +8]
    je      .fatigue_p8
    cmp     r12b, 0x3D          ; bad news [-MORALE -10]
    je      .morale_m10
    cmp     r12b, 0x3F          ; sees something [-MORALE -8]
    je      .morale_m8
    jmp     .done

.fatigue_p15:
    movzx   eax, byte [r13 + DWARF_FATIGUE]
    add     eax, 15
    jmp     .clamp_fatigue
.fatigue_p10:
    movzx   eax, byte [r13 + DWARF_FATIGUE]
    add     eax, 10
    jmp     .clamp_fatigue
.fatigue_p8:
    movzx   eax, byte [r13 + DWARF_FATIGUE]
    add     eax, 8
.clamp_fatigue:
    cmp     eax, FATIGUE_MAX
    jle     .store_fatigue
    mov     eax, FATIGUE_MAX
.store_fatigue:
    mov     [r13 + DWARF_FATIGUE], al
    jmp     .done

.morale_m10:
    movzx   eax, byte [r13 + DWARF_MORALE]
    sub     eax, 10
    jmp     .clamp_morale_low
.morale_m8:
    movzx   eax, byte [r13 + DWARF_MORALE]
    sub     eax, 8
    jmp     .clamp_morale_low
.morale_m7:
    movzx   eax, byte [r13 + DWARF_MORALE]
    sub     eax, 7
    jmp     .clamp_morale_low
.morale_m6:
    movzx   eax, byte [r13 + DWARF_MORALE]
    sub     eax, 6
    jmp     .clamp_morale_low
.morale_m5:
    movzx   eax, byte [r13 + DWARF_MORALE]
    sub     eax, 5
.clamp_morale_low:
    jge     .store_morale_neg
    xor     eax, eax
.store_morale_neg:
    mov     [r13 + DWARF_MORALE], al

.done:
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret