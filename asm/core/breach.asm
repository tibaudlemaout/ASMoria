; =========================================================
; asm/core/breach.asm - Raid warning, combat, and resolution
;
; Exports:
;   asm_tick_breach(state)   - called each tick by tick.asm
;   asm_breach_retreat(state)- player retreats from combat
;
; Raid flow:
;   1. At next_raid_tick: set RAID_WARNING, fire warning event
;   2. At next_raid_tick + RAID_WARN_TICKS: collect guards,
;      compute stats, set RAID_COMBAT
;   3. Every RAID_COMBAT_INTERVAL ticks: exchange damage
;   4. Win (enemy_hp=0) or lose (all guards KO/timer) ->
;      RAID_RESULT, apply outcome, schedule next raid
;
; Guard HP  = 50 + guard_job_level*15 + morale/2
; Guard ATK = 5  + guard_job_level*3
; Enemy stats scale by threat level (1-5)
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  push rbx: 24  push r12: 32
;   push r13: 40  push r14: 48  push r15: 56
;   sub rsp,8: 64  aligned  <- calls here
; =========================================================

%include "core/offsets.inc"

; Enemy stat tables indexed by threat (1-5, index 0 unused)
section .data
enemy_hp_table:  dd 0, 30, 60, 100, 150, 220
enemy_atk_table: dd 0,  3,  6,  10,  15,  22
reward_table:    dd 0, 20, 40,   70, 100, 150

; Event codes
%define EVT_RAID_WARNING    0x47    ; "Something stirs below — assign your guards"
%define EVT_RAID_START      0x48    ; "The tunnels breach. Guards to your stations!"
%define EVT_RAID_WIN        0x49    ; "The raid has been repelled!"
%define EVT_RAID_LOSS       0x4E    ; "The raid overwhelms your defences."
%define EVT_RAID_RETREAT    0x4F    ; "Your guards fall back."

section .text
    global asm_tick_breach
    global asm_breach_retreat

extern asm_event_push

; ---------------------------------------------------------
; asm_tick_breach(rdi=state)
; ---------------------------------------------------------
asm_tick_breach:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi
    lea     r12, [rbx + GS_RAID]        ; r12 = &raid

    movzx   eax, byte [r12 + RAID_ACTIVE]
    cmp     al, RAID_NONE
    je      .check_schedule
    cmp     al, RAID_WARNING
    je      .check_combat_start
    cmp     al, RAID_COMBAT
    je      .do_combat
    jmp     .done                       ; RAID_RESULT — wait for cleanup

    ; -------------------------------------------------------
    ; No active raid — check if it's time to fire a warning
    ; -------------------------------------------------------
.check_schedule:
    mov     rax, [r12 + RAID_NEXT_TICK]
    test    rax, rax
    jnz     .check_time

    ; first run — initialise next_raid_tick relative to current tick
    mov     rax, [rbx + GS_TICK]
    add     rax, RAID_FIRST_TICK
    mov     [r12 + RAID_NEXT_TICK], rax
    jmp     .done

.check_time:
    mov     rcx, [rbx + GS_TICK]
    cmp     rcx, rax
    jl      .done

    ; fire warning
    call    .fire_warning
    jmp     .done

    ; -------------------------------------------------------
    ; Warning phase — wait RAID_WARN_TICKS then start combat
    ; -------------------------------------------------------
.check_combat_start:
    mov     rax, [r12 + RAID_NEXT_TICK]
    add     rax, RAID_WARN_TICKS
    mov     rcx, [rbx + GS_TICK]
    cmp     rcx, rax
    jl      .done

    ; start combat — collect guards
    call    .start_combat
    jmp     .done

    ; -------------------------------------------------------
    ; Combat phase
    ; -------------------------------------------------------
.do_combat:
    ; check timer — auto-loss after RAID_TIMER ticks
    mov     rax, [r12 + RAID_COMBAT_START]
    add     rax, RAID_TIMER
    mov     rcx, [rbx + GS_TICK]
    cmp     rcx, rax
    jge     .combat_loss

    ; only exchange damage every RAID_COMBAT_INTERVAL ticks
    mov     rax, [r12 + RAID_LAST_COMBAT]
    add     rax, RAID_COMBAT_INTERVAL
    cmp     rcx, rax
    jl      .done

    ; update last combat tick
    mov     [r12 + RAID_LAST_COMBAT], rcx

    ; guards attack enemy
    movzx   r13d, byte [r12 + RAID_GUARD_COUNT]
    xor     r14d, r14d                  ; total guard damage

.guard_atk_loop:
    test    r13d, r13d
    jz      .apply_guard_damage
    dec     r13d

    ; skip knocked-out guards (hp <= 0)
    mov     eax, [r12 + RAID_GUARD_HP + r13*4]
    test    eax, eax
    jle     .guard_atk_loop

    ; guard ATK = 5 + guard_job_level*3
    movzx   ecx, byte [r12 + RAID_GUARD_IDX + r13]
    imul    ecx, SIZEOF_DWARF
    lea     rdx, [rbx + GS_DWARVES + rcx]
    movzx   ecx, byte [rdx + DWARF_JOB_LEVEL + JOB_GUARD]
    imul    ecx, 3
    add     ecx, 5
    add     r14d, ecx
    jmp     .guard_atk_loop

.apply_guard_damage:
    sub     [r12 + RAID_ENEMY_HP], r14d
    mov     eax, [r12 + RAID_ENEMY_HP]
    test    eax, eax
    jle     .combat_win

    ; enemy attacks each guard
    mov     eax, [r12 + RAID_ENEMY_ATK]
    movzx   r13d, byte [r12 + RAID_GUARD_COUNT]
    test    r13d, r13d
    jz      .combat_loss               ; no guards = instant loss
    ; split damage evenly
    xor     edx, edx
    div     r13d                        ; eax = dmg per guard (rounded down)
    cmp     eax, 1
    jge     .apply_enemy_dmg
    mov     eax, 1                      ; minimum 1 damage

.apply_enemy_dmg:
    mov     r15d, eax                   ; r15 = per-guard damage
    movzx   r13d, byte [r12 + RAID_GUARD_COUNT]
    xor     r14d, r14d                  ; active guard count

.enemy_dmg_loop:
    test    r13d, r13d
    jz      .check_all_ko
    dec     r13d

    mov     eax, [r12 + RAID_GUARD_HP + r13*4]
    test    eax, eax
    jle     .enemy_dmg_loop             ; already KO

    sub     eax, r15d
    mov     [r12 + RAID_GUARD_HP + r13*4], eax
    inc     r14d                        ; this guard was active

    test    eax, eax
    jg      .enemy_dmg_loop             ; still alive

    ; guard knocked out — apply penalties to dwarf
    movzx   ecx, byte [r12 + RAID_GUARD_IDX + r13]
    imul    ecx, SIZEOF_DWARF
    lea     rdx, [rbx + GS_DWARVES + rcx]
    mov     byte [rdx + DWARF_FATIGUE],  100
    movzx   eax, byte [rdx + DWARF_MORALE]
    sub     eax, 30
    jge     .ko_morale_ok
    xor     eax, eax
.ko_morale_ok:
    mov     [rdx + DWARF_MORALE], al
    jmp     .enemy_dmg_loop

.check_all_ko:
    test    r14d, r14d
    jz      .combat_loss               ; all guards KO

    jmp     .done

.combat_win:
    mov     byte [r12 + RAID_ACTIVE], RAID_RESULT

    ; give reward gold
    mov     eax, [r12 + RAID_REWARD_GOLD]
    add     [rbx + GS_RESOURCES + RES_GOLD], rax

    ; give guards XP
    movzx   r13d, byte [r12 + RAID_GUARD_COUNT]
.win_xp_loop:
    test    r13d, r13d
    jz      .win_event
    dec     r13d
    movzx   ecx, byte [r12 + RAID_GUARD_IDX + r13]
    imul    ecx, SIZEOF_DWARF
    lea     rdx, [rbx + GS_DWARVES + rcx]
    add     qword [rdx + DWARF_JOB_XP + JOB_GUARD*8], 100
    jmp     .win_xp_loop

.win_event:
    mov     rdi, rbx
    mov     rsi, EVT_RAID_WIN
    mov     rdx, EVT_MILESTONE
    mov     rcx, 0xFF
    call    asm_event_push
    call    .schedule_next
    jmp     .done

.combat_loss:
    mov     byte [r12 + RAID_ACTIVE], RAID_RESULT

    ; steal resources proportional to threat
    movzx   eax, byte [r12 + RAID_THREAT]
    imul    eax, 20
    mov     rcx, [rbx + GS_RESOURCES + RES_GOLD]
    cmp     rcx, rax
    jge     .steal_gold
    mov     eax, ecx
.steal_gold:
    sub     [rbx + GS_RESOURCES + RES_GOLD], rax

    ; morale hit to all dwarves
    lea     rdx, [rbx + GS_DWARVES]
    mov     r13d, MAX_DWARVES
.loss_morale_loop:
    test    r13d, r13d
    jz      .loss_event
    dec     r13d
    movzx   eax, byte [rdx + DWARF_ALIVE]
    test    al, al
    jz      .loss_morale_next
    movzx   eax, byte [rdx + DWARF_MORALE]
    sub     eax, 10
    jge     .loss_store_morale
    xor     eax, eax
.loss_store_morale:
    mov     [rdx + DWARF_MORALE], al
.loss_morale_next:
    add     rdx, SIZEOF_DWARF
    jmp     .loss_morale_loop

.loss_event:
    mov     rdi, rbx
    mov     rsi, EVT_RAID_LOSS
    mov     rdx, EVT_NEGATIVE
    mov     rcx, 0xFF
    call    asm_event_push
    call    .schedule_next
    jmp     .done

    ; -------------------------------------------------------
    ; Local helpers
    ; -------------------------------------------------------
.fire_warning:
    ; compute threat: 1 + raids_completed/3 (cap 5)
    ; Only fire if not already active
    movzx   eax, byte [r12 + RAID_ACTIVE]
    test    al, al
    jnz     .fw_ret                     ; already active, skip

    mov     eax, [r12 + RAID_COMPLETED]
    xor     edx, edx
    mov     ecx, 3
    div     ecx
    inc     eax
    cmp     eax, 5
    jle     .threat_ok
    mov     eax, 5
.threat_ok:
    mov     [r12 + RAID_THREAT], al
    mov     byte [r12 + RAID_ACTIVE], RAID_WARNING

    mov     rdi, rbx
    mov     rsi, EVT_RAID_WARNING
    mov     rdx, EVT_NEGATIVE
    mov     rcx, 0xFF
    call    asm_event_push
.fw_ret:
    ret

.start_combat:
    ; collect all alive guards into raid slots
    lea     rdx, [rbx + GS_DWARVES]
    xor     r13d, r13d                  ; slot index
    xor     r14d, r14d                  ; dwarf index

.collect_guards:
    cmp     r14d, MAX_DWARVES
    jge     .guards_collected
    cmp     r13d, RAID_MAX_GUARDS
    jge     .guards_collected

    movzx   eax, byte [rdx + DWARF_ALIVE]
    test    al, al
    jz      .collect_next
    movzx   eax, byte [rdx + DWARF_JOB]
    cmp     al, JOB_GUARD
    jne     .collect_next

    ; found a guard — compute HP and ATK
    mov     [r12 + RAID_GUARD_IDX + r13], r14b

    ; HP = 50 + level*15 + morale/2
    movzx   eax, byte [rdx + DWARF_JOB_LEVEL + JOB_GUARD]
    imul    eax, 15
    add     eax, 50
    movzx   ecx, byte [rdx + DWARF_MORALE]
    shr     ecx, 1
    add     eax, ecx
    mov     [r12 + RAID_GUARD_HP     + r13*4], eax
    mov     [r12 + RAID_GUARD_HP_MAX + r13*4], eax

    inc     r13d

.collect_next:
    add     rdx, SIZEOF_DWARF
    inc     r14d
    jmp     .collect_guards

.guards_collected:
    mov     [r12 + RAID_GUARD_COUNT], r13b

    ; if no guards — immediate loss
    test    r13d, r13d
    jz      .no_guards_loss

    ; set enemy stats from threat table
    movzx   eax, byte [r12 + RAID_THREAT]
    lea     rcx, [rel enemy_hp_table]
    mov     edx, [rcx + rax*4]
    mov     [r12 + RAID_ENEMY_HP],     edx
    mov     [r12 + RAID_ENEMY_HP_MAX], edx

    lea     rcx, [rel enemy_atk_table]
    mov     edx, [rcx + rax*4]
    mov     [r12 + RAID_ENEMY_ATK], edx

    lea     rcx, [rel reward_table]
    mov     edx, [rcx + rax*4]
    mov     [r12 + RAID_REWARD_GOLD], edx

    ; start combat
    mov     byte [r12 + RAID_ACTIVE], RAID_COMBAT
    mov     rax, [rbx + GS_TICK]
    mov     [r12 + RAID_COMBAT_START], rax
    mov     [r12 + RAID_LAST_COMBAT],  rax

    mov     rdi, rbx
    mov     rsi, EVT_RAID_START
    mov     rdx, EVT_NEGATIVE
    mov     rcx, 0xFF
    call    asm_event_push
    ret

.no_guards_loss:
    ; no guards assigned — resources stolen immediately
    movzx   eax, byte [r12 + RAID_THREAT]
    imul    eax, 20
    mov     rcx, [rbx + GS_RESOURCES + RES_GOLD]
    cmp     rcx, rax
    jge     .no_guard_steal
    mov     eax, ecx
.no_guard_steal:
    sub     [rbx + GS_RESOURCES + RES_GOLD], rax
    mov     byte [r12 + RAID_ACTIVE], RAID_RESULT
    mov     rdi, rbx
    mov     rsi, EVT_RAID_LOSS
    mov     rdx, EVT_NEGATIVE
    mov     rcx, 0xFF
    call    asm_event_push
    call    .schedule_next
    ret

.schedule_next:
    inc     dword [r12 + RAID_COMPLETED]
    mov     rax, [rbx + GS_TICK]
    add     rax, RAID_INTERVAL
    mov     [r12 + RAID_NEXT_TICK], rax
    mov     byte [r12 + RAID_ACTIVE], RAID_NONE
    ret

.done:
    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

; ---------------------------------------------------------
; asm_breach_retreat(rdi=state)
; Player retreats — partial loss, guards uninjured
; STACK: same as asm_tick_breach
; ---------------------------------------------------------
asm_breach_retreat:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi
    lea     r12, [rbx + GS_RAID]

    movzx   eax, byte [r12 + RAID_ACTIVE]
    cmp     al, RAID_COMBAT
    jne     .retreat_done               ; can only retreat during combat

    ; partial gold loss
    movzx   eax, byte [r12 + RAID_THREAT]
    imul    eax, 10                     ; half penalty vs full loss
    mov     rcx, [rbx + GS_RESOURCES + RES_GOLD]
    cmp     rcx, rax
    jge     .retreat_steal
    mov     eax, ecx
.retreat_steal:
    sub     [rbx + GS_RESOURCES + RES_GOLD], rax

    mov     rdi, rbx
    mov     rsi, EVT_RAID_RETREAT
    mov     rdx, EVT_NEGATIVE
    mov     rcx, 0xFF
    call    asm_event_push

    ; schedule next raid
    inc     dword [r12 + RAID_COMPLETED]
    mov     rax, [rbx + GS_TICK]
    add     rax, RAID_INTERVAL
    mov     [r12 + RAID_NEXT_TICK], rax
    mov     byte [r12 + RAID_ACTIVE], RAID_NONE

.retreat_done:
    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret