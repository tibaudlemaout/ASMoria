; =========================================================
; asm/core/breach.asm - Tower-defence raid system
;
; Exports:
;   asm_tick_breach(rdi=state)
;   asm_breach_place(rdi=state, rsi=col, rdx=row, rcx=type)
;   asm_breach_retreat(rdi=state)
;
; Raid flow:
;   RAID_NONE    -> at next_raid_tick: set RAID_WARNING
;   RAID_WARNING -> player places guards/walls/traps
;                   at next_raid_tick+WARN_TICKS: spawn enemies, RAID_COMBAT
;   RAID_COMBAT  -> every tick: tick move timers, move enemies,
;                   resolve traps/guard combat/wall attacks,
;                   check win/lose
;   RAID_RESULT  -> award gold/relics, schedule next raid
;
; Grid: grid[row][col], col 0=spawn col 11=settlement
; =========================================================

%include "core/offsets.inc"

; tier2 is the second uint64_t in the Upgrades struct
%ifndef GS_UPGR_TIER2
%define GS_UPGR_TIER2  GS_UPGR_TIER1 + 8
%endif

; Crafted breach item resource offsets (Resources struct, each field 8 bytes)
; walls=index 14, spike_traps=15, slow_traps=16 → offsets 0x70, 0x78, 0x80
%ifndef RES_WALLS
%define RES_WALLS       0x70
%endif
%ifndef RES_SPIKE_TRAPS
%define RES_SPIKE_TRAPS 0x78
%endif
%ifndef RES_SLOW_TRAPS
%define RES_SLOW_TRAPS  0x80
%endif

extern asm_rng_next
extern asm_tavern_buff_active

; Enemy stat tables [threat 0-5, index 0 unused]
section .data

; ---------------------------------------------------------------
; Raid-type tables — indexed by (raid_type * 2 + variant)
; variant = 0 for threat 1-2 (weak), 1 for threat 3+ (strong)
; Layout: [goblin_w, goblin_s, troll_w, troll_s,
;           necro_w,  necro_s,  dragon_w, dragon_s]
;
; Goblin:      Scout / Raider   (familiar foes, low depth)
; Troll:       Stone / War      (tough, mid depth)
; Necromancer: Skeleton / Lich  (fast chaff or slow powerhouse)
; Dragon:      Drake / Dragon   (rare, high depth, deadly)
; ---------------------------------------------------------------
rt_hp_max:   dw  30,  50, 120, 200,  20,  90, 180, 350
rt_atk:      db   5,   8,  15,  22,   7,  20,  30,  45
rt_move_int: db   8,   8,  12,  12,   6,  15,  10,  14
rt_type:     db   ENEMY_GOBLIN_SCOUT, ENEMY_GOBLIN_RAIDER, \
                  ENEMY_STONE_TROLL,  ENEMY_WAR_TROLL,     \
                  ENEMY_SKELETON,     ENEMY_LICH,           \
                  ENEMY_DRAKE,        ENEMY_DRAGON
; enemy count per (raid_type*2+variant)
rt_count:    db   2,   4,   1,   3,   3,   5,   1,   2

; Reward tables [threat 1-5]
reward_gold_tab:   dd 0, 50, 100, 200, 350, 600
reward_relics_tab: dd 0,  0,   0,   5,  10,  20

section .text
    global asm_tick_breach
    global asm_breach_place
    global asm_collect_guards
    global asm_breach_retreat

; ---------------------------------------------------------
; compute_threat(rbx=state) -> al = threat (1-5)
; Based on depth, tick count, and dwarf count
; ---------------------------------------------------------
compute_threat:
    push    rcx
    push    rdx

    ; base = depth
    movzx   eax, byte [rbx + GS_DEPTH]
    cmp     eax, 5
    jle     .depth_ok
    mov     eax, 5
.depth_ok:
    ; +1 if tick > 1000
    mov     rcx, [rbx + GS_TICK]
    cmp     rcx, 1000
    jl      .no_tick_bonus
    inc     eax
.no_tick_bonus:
    ; +1 if dwarf count > 24
    xor     ecx, ecx
    lea     rdx, [rbx + GS_DWARVES]
    mov     r8d, MAX_DWARVES
.count_loop:
    test    r8d, r8d
    jz      .count_done
    movzx   r9d, byte [rdx + DWARF_ALIVE]
    add     ecx, r9d
    add     rdx, SIZEOF_DWARF
    dec     r8d
    jmp     .count_loop
.count_done:
    cmp     ecx, 24
    jl      .no_dwarf_bonus
    inc     eax
.no_dwarf_bonus:
    ; clamp 1-5
    cmp     eax, 1
    jge     .min_ok
    mov     eax, 1
.min_ok:
    cmp     eax, 5
    jle     .max_ok
    mov     eax, 5
.max_ok:
    pop     rdx
    pop     rcx
    ret

; ---------------------------------------------------------
; init_grid(rbx=state) — ensure settlement column is marked
; Does NOT wipe placed guards/walls/traps so pre-placed
; defences survive into combat.
; ---------------------------------------------------------
init_grid:
    push    rdi
    push    rcx

    ; Only mark settlement column -- preserve everything else
    mov     ecx, RAID_ROWS
    lea     rdi, [rbx + GS_RAID + RAID_GRID]
.settle_loop:
    test    ecx, ecx
    jz      .settle_done
    mov     byte [rdi + RAID_COL_SETTLE], CELL_SETTLEMENT
    add     rdi, RAID_COLS
    dec     ecx
    jmp     .settle_loop
.settle_done:
    pop     rcx
    pop     rdi
    ret

; ---------------------------------------------------------
; spawn_enemies(rbx=state) — populate enemy list based on threat
; ---------------------------------------------------------
spawn_enemies:
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

    lea     r12, [rbx + GS_RAID]

    ; compute table index = raid_type * 2 + variant
    ; variant = (threat >= 3) ? 1 : 0
    movzx   eax, byte [r12 + RAID_THREAT]
    movzx   r10d, byte [r12 + RAID_TYPE]
    imul    r10d, 2                ; r10 = raid_type * 2
    cmp     eax, 3
    jl      .se_weak
    inc     r10d                   ; strong variant
.se_weak:
    ; r10 = final table index (0-7)

    ; load stats for this raid type + variant
    push    rcx
    lea     rcx, [rel rt_count]
    movzx   r9d, byte [rcx + r10]         ; r9 = enemy count
    lea     rcx, [rel rt_hp_max]
    movzx   r11d, word [rcx + r10*2]      ; r11 = hp_max
    pop     rcx

    lea     r8, [r12 + RAID_ENEMIES]
    push    rdi
    push    rcx
    mov     rdi, r8
    mov     ecx, RAID_MAX_ENEMIES * SIZEOF_ENEMY
    xor     eax, eax
    rep     stosb
    pop     rcx
    pop     rdi

    mov     byte [r12 + RAID_ENEMIES_REMAINING], r9b

    xor     ecx, ecx
.spawn_loop:
    cmp     ecx, r9d
    jge     .spawn_done
    cmp     ecx, RAID_MAX_ENEMIES
    jge     .spawn_done

    imul    eax, ecx, SIZEOF_ENEMY
    lea     rdi, [r8 + rax]

    ; pick row via rng
    push    rdi
    push    rcx
    push    r8
    push    r9
    push    r10
    push    r11
    mov     rdi, rbx
    call    asm_rng_next
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rcx
    pop     rdi

    xor     edx, edx
    mov     r13d, RAID_ROWS
    div     r13                    ; rdx = row

    ; load enemy type, atk, move_int from tables using r10 index
    push    rcx
    lea     rcx, [rel rt_type]
    movzx   r13d, byte [rcx + r10]         ; enemy type
    lea     rcx, [rel rt_atk]
    movzx   r14d, byte [rcx + r10]         ; atk
    lea     rcx, [rel rt_move_int]
    movzx   r15d, byte [rcx + r10]         ; move interval
    pop     rcx

    mov     byte [rdi + ENEMY_TYPE],       r13b
    mov     byte [rdi + ENEMY_COL],        0
    mov     byte [rdi + ENEMY_ROW],        dl
    mov     byte [rdi + ENEMY_SLOW_TIMER], 0
    mov     word [rdi + ENEMY_HP],         r11w
    mov     word [rdi + ENEMY_HP_MAX],     r11w
    mov     byte [rdi + ENEMY_ATK],        r14b
    mov     byte [rdi + ENEMY_MOVE_TIMER], r15b
    mov     byte [rdi + ENEMY_MOVE_INT],   r15b   ; stored for tick_enemies

    imul    eax, ecx, 10
    mov     byte [rdi + ENEMY_SPAWN_DELAY], al

    inc     ecx
    jmp     .spawn_loop
.spawn_done:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    ret

; ---------------------------------------------------------
; asm_collect_guards / collect_guards(rbx=state) — find all guard-job dwarves and
; add them to raid->guards if they have been placed on grid
; Also initialise any guards found placed on grid
; ---------------------------------------------------------
asm_collect_guards:
    push    rbx
    mov     rbx, rdi
    call    collect_guards
    pop     rbx
    ret

collect_guards:
    push    r8
    push    r9
    push    r10
    push    r11

    lea     r8, [rbx + GS_RAID + RAID_GUARDS]
    xor     r9d, r9d    ; guard count

    lea     r10, [rbx + GS_DWARVES]
    xor     r11d, r11d  ; dwarf index
.cg_loop:
    cmp     r11d, MAX_DWARVES
    jge     .cg_done
    cmp     r9d, RAID_MAX_GUARDS
    jge     .cg_done

    movzx   eax, byte [r10 + DWARF_ALIVE]
    test    eax, eax
    jz      .cg_next

    movzx   eax, byte [r10 + DWARF_JOB]
    cmp     eax, JOB_GUARD
    jne     .cg_next

    ; compute HP and ATK
    movzx   eax, byte [r10 + DWARF_JOB_LEVEL + JOB_GUARD]
    imul    eax, 15
    add     eax, 50
    movzx   ecx, byte [r10 + DWARF_MORALE]
    shr     ecx, 1
    add     eax, ecx
    ; armour bonus
    movzx   ecx, byte [r10 + DWARF_EQUIPMENT]
    cmp     ecx, EQUIP_ARMOUR
    jne     .no_arm
    add     eax, ARMOUR_HP_BONUS
.no_arm:
    ; warrior's draft HP bonus
    ; r8/r9/r10/r11 hold this loop's live state (guard array base,
    ; guard count/index, dwarf array ptr, dwarf index) and are
    ; caller-saved under the ABI -- asm_tavern_buff_active is free to
    ; clobber them, so they MUST be saved across the call or the loop
    ; silently corrupts itself after returning.
    push    r8
    push    r9
    push    r10
    push    r11
    push    rdi
    push    rsi
    push    rax
    mov     rdi, rbx
    mov     esi, BUFF_WARRIORS_DRAFT
    call    asm_tavern_buff_active
    pop     rax
    test    eax, eax
    jz      .no_draft_hp
    add     eax, 15
.no_draft_hp:
    pop     rsi
    pop     rdi
    pop     r11
    pop     r10
    pop     r9
    pop     r8

    ; get guard slot
    imul    ecx, r9d, SIZEOF_RAIDGUARD
    lea     rdx, [r8 + rcx]

    mov     byte [rdx + RGUARD_DWARF_IDX], r11b
    mov     byte [rdx + RGUARD_ACTIVE],    1

    ; only set col/row if not already placed (preserve placements)
    movzx   ecx, byte [rdx + RGUARD_COL]
    cmp     ecx, 0xFF
    jne     .cg_already_placed  ; already has a position, keep it
    ; newly seen guard: mark as unplaced
    mov     byte [rdx + RGUARD_COL], 0xFF
.cg_already_placed:

    ; store HP
    mov     word [rdx + RGUARD_HP],     ax
    mov     word [rdx + RGUARD_HP_MAX], ax

    ; TRAIT_IRONHIDE: +20 max HP for hero guards
    movzx   ecx, byte [r10 + DWARF_IS_HERO]
    test    ecx, ecx
    jz      .cg_no_ironhide
    movzx   ecx, byte [r10 + DWARF_HERO_TRAIT]
    cmp     ecx, TRAIT_IRONHIDE
    jne     .cg_no_ironhide
    add     word [rdx + RGUARD_HP],     20
    add     word [rdx + RGUARD_HP_MAX], 20
.cg_no_ironhide:

    inc     r9d

.cg_next:
    add     r10, SIZEOF_DWARF
    inc     r11d
    jmp     .cg_loop
.cg_done:
    lea     r8, [rbx + GS_RAID]
    mov     byte [r8 + RAID_GUARD_COUNT], r9b

    pop     r11
    pop     r10
    pop     r9
    pop     r8
    ret

; ---------------------------------------------------------
; tick_enemies(rbx=state) — move enemies, resolve collisions
; Called every tick during RAID_COMBAT
; ---------------------------------------------------------
tick_enemies:
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13

    lea     r12, [rbx + GS_RAID]

    xor     r13d, r13d  ; enemy index
.te_loop:
    cmp     r13d, RAID_MAX_ENEMIES
    jge     .te_done

    imul    eax, r13d, SIZEOF_ENEMY
    lea     r8, [r12 + RAID_ENEMIES + rax]

    ; skip inactive enemies
    movzx   eax, byte [r8 + ENEMY_TYPE]
    test    eax, eax
    jz      .te_next

    ; handle spawn delay
    movzx   eax, byte [r8 + ENEMY_SPAWN_DELAY]
    test    eax, eax
    jz      .te_active
    dec     eax
    mov     byte [r8 + ENEMY_SPAWN_DELAY], al
    jmp     .te_next

.te_active:
    ; tick move timer
    movzx   eax, byte [r8 + ENEMY_MOVE_TIMER]
    test    eax, eax
    jz      .te_move
    dec     eax
    mov     byte [r8 + ENEMY_MOVE_TIMER], al
    jmp     .te_next

.te_move:
    ; reset move timer from ENEMY_MOVE_INT stored at spawn
    ; slow_timer doubles the interval when active
    movzx   ecx, byte [r8 + ENEMY_SLOW_TIMER]
    test    ecx, ecx
    jz      .te_normal_speed
    dec     ecx
    mov     byte [r8 + ENEMY_SLOW_TIMER], cl
    movzx   eax, byte [r8 + ENEMY_MOVE_INT]
    shl     eax, 1              ; *2 when slowed
    mov     byte [r8 + ENEMY_MOVE_TIMER], al
    jmp     .te_do_move

.te_normal_speed:
    movzx   eax, byte [r8 + ENEMY_MOVE_INT]
    mov     byte [r8 + ENEMY_MOVE_TIMER], al

.te_do_move:
    ; try to move right (col+1)
    movzx   eax, byte [r8 + ENEMY_COL]
    movzx   ecx, byte [r8 + ENEMY_ROW]

    ; check col+1
    mov     edx, eax
    inc     edx
    cmp     edx, RAID_COLS
    jge     .te_next    ; out of bounds (shouldn't happen)

    ; check what's at grid[row][col+1]
    imul    r9d, ecx, RAID_COLS
    add     r9d, edx
    lea     r10, [r12 + RAID_GRID]
    movzx   r9d, byte [r10 + r9]

    cmp     r9d, CELL_SETTLEMENT
    je      .te_breach

    cmp     r9d, CELL_WALL
    je      .te_attack_wall

    cmp     r9d, CELL_GUARD
    je      .te_attack_guard

    cmp     r9d, CELL_SPIKE_TRAP
    je      .te_spike_trap

    cmp     r9d, CELL_SLOW_TRAP
    je      .te_slow_trap

    ; empty cell — move into it
    ; clear old cell
    imul    r9d, ecx, RAID_COLS
    add     r9d, eax
    mov     byte [r10 + r9], CELL_EMPTY

    ; move enemy
    mov     byte [r8 + ENEMY_COL], dl

    ; set new cell (enemy presence not tracked in grid, only placed objects)
    jmp     .te_next

.te_breach:
    ; enemy reached settlement
    mov     byte [r12 + RAID_SETTLEMENT_BREACH], 1
    ; kill this enemy entry
    mov     byte [r8 + ENEMY_TYPE], ENEMY_NONE
    ; decrement enemies_remaining
    movzx   eax, byte [r12 + RAID_ENEMIES_REMAINING]
    test    eax, eax
    jz      .te_next
    dec     eax
    mov     byte [r12 + RAID_ENEMIES_REMAINING], al
    jmp     .te_next

.te_spike_trap:
    ; take 40 damage, remove trap
    movsx   eax, word [r8 + ENEMY_HP]
    sub     eax, 40
    mov     word [r8 + ENEMY_HP], ax
    ; remove trap from grid
    imul    r9d, ecx, RAID_COLS
    add     r9d, edx
    mov     byte [r10 + r9], CELL_EMPTY
    ; also remove from wall list? (trap list embedded in grid)
    ; check if enemy died
    test    ax, ax
    jg      .te_move_onto_trap
    ; enemy dead
    call    enemy_die
    jmp     .te_next
.te_move_onto_trap:
    ; move enemy onto now-empty cell
    imul    r9d, ecx, RAID_COLS
    add     r9d, eax
    mov     byte [r10 + r9], CELL_EMPTY
    mov     byte [r8 + ENEMY_COL], dl
    jmp     .te_next

.te_slow_trap:
    ; apply slow, remove trap
    mov     byte [r8 + ENEMY_SLOW_TIMER], RAID_SLOW_TURNS
    imul    r9d, ecx, RAID_COLS
    add     r9d, edx
    mov     byte [r10 + r9], CELL_EMPTY
    ; move onto cell
    imul    r9d, ecx, RAID_COLS
    movzx   eax, byte [r8 + ENEMY_COL]
    add     r9d, eax
    mov     byte [r10 + r9], CELL_EMPTY
    mov     byte [r8 + ENEMY_COL], dl
    jmp     .te_next

.te_attack_wall:
    ; find wall at (row=ecx, col=edx) and damage it
    push    r14
    push    r15
    lea     r14, [r12 + RAID_WALLS]
    movzx   r15d, byte [r12 + RAID_WALL_COUNT]
    xor     r11d, r11d
.wall_find:
    cmp     r11d, r15d
    jge     .wall_done
    imul    eax, r11d, SIZEOF_RAIDWALL
    lea     r9, [r14 + rax]
    movzx   eax, byte [r9 + RWALL_COL]
    cmp     eax, edx
    jne     .wall_next
    movzx   eax, byte [r9 + RWALL_ROW]
    cmp     eax, ecx
    jne     .wall_next
    ; found wall — apply enemy ATK as damage
    movsx   eax, word [r9 + RWALL_HP]
    movsx   r10d, byte [r8 + ENEMY_ATK]
    sub     eax, r10d
    mov     word [r9 + RWALL_HP], ax
    ; if wall destroyed, clear grid cell
    test    ax, ax
    jg      .wall_done
    ; remove wall
    imul    r10d, ecx, RAID_COLS
    add     r10d, edx
    lea     r11, [r12 + RAID_GRID]
    mov     byte [r11 + r10], CELL_EMPTY
    ; compact wall list
    ; (copy last wall to this slot, decrement count)
    dec     r15d
    mov     byte [r12 + RAID_WALL_COUNT], r15b
    cmp     r11d, r15d
    je      .wall_done
    imul    eax, r15d, SIZEOF_RAIDWALL
    lea     r10, [r14 + rax]
    mov     eax, dword [r10]
    mov     dword [r9], eax
    jmp     .wall_done
.wall_next:
    inc     r11d
    jmp     .wall_find
.wall_done:
    pop     r15
    pop     r14
    jmp     .te_next

.te_attack_guard:
    ; find guard at (row=ecx, col=edx) and fight
    push    r14
    push    r15
    lea     r14, [r12 + RAID_GUARDS]
    movzx   r15d, byte [r12 + RAID_GUARD_COUNT]
    xor     r11d, r11d
.guard_find:
    cmp     r11d, r15d
    jge     .guard_done
    imul    eax, r11d, SIZEOF_RAIDGUARD
    lea     r9, [r14 + rax]
    movzx   eax, byte [r9 + RGUARD_ACTIVE]
    test    eax, eax
    jz      .guard_next
    movzx   eax, byte [r9 + RGUARD_COL]
    cmp     eax, edx
    jne     .guard_next
    movzx   eax, byte [r9 + RGUARD_ROW]
    cmp     eax, ecx
    jne     .guard_next

    ; guard takes enemy ATK damage
    movsx   eax, word [r9 + RGUARD_HP]
    movsx   r10d, byte [r8 + ENEMY_ATK]
    sub     eax, r10d
    mov     word [r9 + RGUARD_HP], ax

    ; enemy takes guard ATK damage
    ; compute guard ATK from dwarf stats
    movzx   r10d, byte [r9 + RGUARD_DWARF_IDX]
    imul    r10d, SIZEOF_DWARF
    lea     r10, [rbx + GS_DWARVES + r10]
    movzx   r10d, byte [r10 + DWARF_JOB_LEVEL + JOB_GUARD]
    imul    r10d, 3
    add     r10d, 5
    ; weapon bonus
    movzx   r11d, byte [r9 + RGUARD_DWARF_IDX]
    imul    r11d, SIZEOF_DWARF
    lea     r11, [rbx + GS_DWARVES + r11]
    movzx   r11d, byte [r11 + DWARF_EQUIPMENT]
    cmp     r11d, EQUIP_WEAPON
    jne     .no_wpn
    add     r10d, WEAPON_ATK_BONUS
.no_wpn:
    ; TRAIT_BERSERKER: double ATK when guard HP < 30% of max
    ; r11 = guard_find loop index (safe to repurpose — guard found, loop ends here)
    movzx   r11d, byte [r9 + RGUARD_DWARF_IDX]
    imul    r11d, SIZEOF_DWARF
    lea     r11, [rbx + GS_DWARVES + r11]
    movzx   eax, byte [r11 + DWARF_IS_HERO]
    test    eax, eax
    jz      .no_berserker
    movzx   eax, byte [r11 + DWARF_HERO_TRAIT]
    cmp     eax, TRAIT_BERSERKER
    jne     .no_berserker
    ; hp * 10 < hp_max * 3  →  HP below 30%
    movsx   eax, word [r9 + RGUARD_HP]
    movsx   ecx, word [r9 + RGUARD_HP_MAX]
    imul    eax, 10
    imul    ecx, 3
    cmp     eax, ecx
    jge     .no_berserker
    shl     r10d, 1             ; double ATK
.no_berserker:
    movsx   eax, word [r8 + ENEMY_HP]
    sub     eax, r10d
    mov     word [r8 + ENEMY_HP], ax

    ; check guard death
    movsx   eax, word [r9 + RGUARD_HP]
    test    eax, eax
    jg      .check_enemy_death
    mov     byte [r9 + RGUARD_ACTIVE], 0
    ; clear guard from grid
    imul    eax, ecx, RAID_COLS
    add     eax, edx
    lea     r10, [r12 + RAID_GRID]
    mov     byte [r10 + rax], CELL_EMPTY
    ; enemy can now move onto the cell
    movzx   eax, byte [r8 + ENEMY_COL]
    imul    eax, RAID_COLS
    movzx   r10d, byte [r8 + ENEMY_ROW]
    ; clear old pos not strictly needed — grid tracks placed objects
    mov     byte [r8 + ENEMY_COL], dl

.check_enemy_death:
    movsx   eax, word [r8 + ENEMY_HP]
    test    eax, eax
    jg      .guard_done
    ; enemy died
    call    enemy_die
    jmp     .guard_done

.guard_next:
    inc     r11d
    jmp     .guard_find
.guard_done:
    pop     r15
    pop     r14
    jmp     .te_next

.te_next:
    inc     r13d
    jmp     .te_loop
.te_done:
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    ret

; ---------------------------------------------------------
; enemy_die — called when enemy HP <= 0
; r8 = enemy ptr, r12 = raid ptr, rbx = state
; Decrements enemies_remaining, awards partial reward
; ---------------------------------------------------------
enemy_die:
    mov     byte [r8 + ENEMY_TYPE], ENEMY_NONE
    movzx   eax, byte [r12 + RAID_ENEMIES_REMAINING]
    test    eax, eax
    jz      .ed_done
    dec     eax
    mov     byte [r12 + RAID_ENEMIES_REMAINING], al
.ed_done:
    ret

; ---------------------------------------------------------
; asm_breach_place(rdi=state, rsi=col, rdx=row, rcx=cell_type)
; Places a guard/wall/trap on the grid during WARNING phase
; Consumes the appropriate resource
; Returns rax=1 on success, 0 on fail
; ---------------------------------------------------------
asm_breach_place:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 8

    mov     rbx, rdi
    mov     r12d, esi           ; col
    mov     r13d, edx           ; row
    mov     r14d, ecx           ; cell_type

    ; must be in WARNING or NONE phase
    lea     r15, [rbx + GS_RAID]
    movzx   eax, byte [r15 + RAID_ACTIVE]
    cmp     eax, RAID_WARNING
    je      .place_phase_ok
    cmp     eax, RAID_NONE
    jne     .place_fail
.place_phase_ok:

    ; bounds check
    cmp     r12d, 0
    jle     .place_fail         ; can't place at col 0 (spawn) or settlement
    cmp     r12d, RAID_COL_SETTLE
    jge     .place_fail
    cmp     r13d, 0
    jl      .place_fail
    cmp     r13d, RAID_ROWS
    jge     .place_fail

    ; get current cell
    imul    eax, r13d, RAID_COLS
    add     eax, r12d
    lea     rdx, [r15 + RAID_GRID]
    movzx   ecx, byte [rdx + rax]

    cmp     r14d, CELL_GUARD
    je      .place_guard

    cmp     r14d, CELL_WALL
    je      .place_wall

    cmp     r14d, CELL_SPIKE_TRAP
    je      .place_spike

    cmp     r14d, CELL_SLOW_TRAP
    je      .place_slow

    ; CELL_EMPTY = remove whatever is here
    mov     byte [rdx + rax], CELL_EMPTY
    mov     eax, 1
    jmp     .place_done

.place_guard:
    ; find an unplaced guard
    lea     r8, [r15 + RAID_GUARDS]
    movzx   r9d, byte [r15 + RAID_GUARD_COUNT]
    xor     r10d, r10d
.pg_loop:
    cmp     r10d, r9d
    jge     .place_fail
    imul    ecx, r10d, SIZEOF_RAIDGUARD
    lea     r11, [r8 + rcx]
    movzx   ecx, byte [r11 + RGUARD_ACTIVE]
    test    ecx, ecx
    jz      .pg_next
    ; check if unplaced (0xFF = unplaced marker)
    movzx   ecx, byte [r11 + RGUARD_COL]
    cmp     ecx, 0xFF
    je      .pg_found
.pg_next:
    inc     r10d
    jmp     .pg_loop
.pg_found:
    ; place guard
    imul    ecx, r13d, RAID_COLS
    add     ecx, r12d
    lea     rdx, [r15 + RAID_GRID]
    ; check cell is empty
    movzx   eax, byte [rdx + rcx]
    test    eax, eax
    jnz     .place_fail
    mov     byte [rdx + rcx], CELL_GUARD
    imul    ecx, r10d, SIZEOF_RAIDGUARD
    lea     r11, [r8 + rcx]
    mov     byte [r11 + RGUARD_COL], r12b
    mov     byte [r11 + RGUARD_ROW], r13b
    mov     eax, 1
    jmp     .place_done

.place_wall:
    ; consume 1 crafted wall from stockpile (craft via Workshop)
    cmp     qword [rbx + GS_RESOURCES + RES_WALLS], 1
    jl      .place_fail
    ; check wall count
    movzx   eax, byte [r15 + RAID_WALL_COUNT]
    cmp     eax, RAID_MAX_WALLS
    jge     .place_fail
    ; check cell empty
    imul    ecx, r13d, RAID_COLS
    add     ecx, r12d
    lea     rdx, [r15 + RAID_GRID]
    movzx   r8d, byte [rdx + rcx]
    test    r8d, r8d
    jnz     .place_fail
    ; place wall
    sub     qword [rbx + GS_RESOURCES + RES_WALLS], 1
    mov     byte [rdx + rcx], CELL_WALL
    movzx   eax, byte [r15 + RAID_WALL_COUNT]
    imul    ecx, eax, SIZEOF_RAIDWALL
    lea     r8, [r15 + RAID_WALLS + rcx]
    mov     byte [r8 + RWALL_COL], r12b
    mov     byte [r8 + RWALL_ROW], r13b
    mov     word [r8 + RWALL_HP],  50
    inc     eax
    mov     byte [r15 + RAID_WALL_COUNT], al
    mov     eax, 1
    jmp     .place_done

.place_spike:
    ; consume 1 crafted spike trap from stockpile (craft via Workshop)
    cmp     qword [rbx + GS_RESOURCES + RES_SPIKE_TRAPS], 1
    jl      .place_fail
    imul    ecx, r13d, RAID_COLS
    add     ecx, r12d
    lea     rdx, [r15 + RAID_GRID]
    movzx   eax, byte [rdx + rcx]
    test    eax, eax
    jnz     .place_fail
    sub     qword [rbx + GS_RESOURCES + RES_SPIKE_TRAPS], 1
    mov     byte [rdx + rcx], CELL_SPIKE_TRAP
    mov     eax, 1
    jmp     .place_done

.place_slow:
    ; consume 1 crafted slow trap from stockpile (craft via Workshop)
    cmp     qword [rbx + GS_RESOURCES + RES_SLOW_TRAPS], 1
    jl      .place_fail
    imul    ecx, r13d, RAID_COLS
    add     ecx, r12d
    lea     rdx, [r15 + RAID_GRID]
    movzx   eax, byte [rdx + rcx]
    test    eax, eax
    jnz     .place_fail
    sub     qword [rbx + GS_RESOURCES + RES_SLOW_TRAPS], 1
    mov     byte [rdx + rcx], CELL_SLOW_TRAP
    mov     eax, 1
    jmp     .place_done

.place_fail:
    xor     eax, eax
.place_done:
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
; ---------------------------------------------------------
asm_breach_retreat:
    lea     rax, [rdi + GS_RAID]
    movzx   ecx, byte [rax + RAID_ACTIVE]
    cmp     ecx, RAID_COMBAT
    jne     .ret_done
    mov     byte [rax + RAID_ACTIVE], RAID_RESULT
    mov     byte [rax + RAID_SETTLEMENT_BREACH], 1
.ret_done:
    ret

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
    lea     r12, [rbx + GS_RAID]

    movzx   eax, byte [r12 + RAID_ACTIVE]

    cmp     eax, RAID_NONE
    je      .tick_none

    cmp     eax, RAID_WARNING
    je      .tick_warning

    cmp     eax, RAID_COMBAT
    je      .tick_combat

    cmp     eax, RAID_RESULT
    je      .tick_result

    jmp     .tick_done

; -------------------------------------------------------
.tick_none:
    mov     rax, [rbx + GS_TICK]
    mov     rcx, [r12 + RAID_NEXT_TICK]
    cmp     rax, rcx
    jl      .tick_done

    ; trigger warning
    call    compute_threat
    mov     byte [r12 + RAID_THREAT], al

    ; pick raid type: available types = min(depth, 4), chosen by rng % available
    ; depth 1 → goblin only; depth 2 → +troll; depth 3 → +necro; depth 4+ → +dragon
    push    rax                 ; preserve compute_threat result (already stored)
    push    rdi
    movzx   eax, byte [rbx + GS_DEPTH]
    cmp     eax, RAID_TYPE_COUNT
    jle     .rt_clamp
    mov     eax, RAID_TYPE_COUNT
.rt_clamp:
    push    rax                 ; push count through stack — immune to rng_next clobbering
    mov     rdi, rbx
    call    asm_rng_next        ; rax = 64-bit random
    pop     rcx                 ; rcx = count (1..4)
    test    ecx, ecx
    jz      .no_raid_type       ; safety: depth=0 guard (shouldn't happen)
    xor     edx, edx            ; 32-bit div: edx:eax / ecx
    div     ecx                 ; edx = eax % ecx
    mov     byte [r12 + RAID_TYPE], dl
.no_raid_type:
    pop     rdi
    pop     rax

    mov     byte [r12 + RAID_ACTIVE], RAID_WARNING
    mov     byte [r12 + RAID_SETTLEMENT_BREACH], 0
    mov     byte [r12 + RAID_WALL_COUNT], 0

    ; store current tick as warning start (used in tick_warning timer)
    mov     rax, [rbx + GS_TICK]
    mov     [r12 + RAID_LAST_MOVE_TICK], rax

    ; init grid and collect guards for placement
    call    init_grid
    call    collect_guards

    ; fire warning event
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x30
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_NEGATIVE
    mov     byte [rbx + GS_PENDING + PENDING_DWARF],    0xFF
    jmp     .tick_done

; -------------------------------------------------------
.tick_warning:
    ; wait RAID_WARN_TICKS from when warning started (stored in LAST_MOVE_TICK)
    mov     rax, [rbx + GS_TICK]
    mov     rcx, [r12 + RAID_LAST_MOVE_TICK]
    add     rcx, RAID_WARN_TICKS
    cmp     rax, rcx
    jl      .tick_done

    ; spawn enemies and begin combat
    call    spawn_enemies
    mov     byte [r12 + RAID_ACTIVE], RAID_COMBAT
    mov     rax, [rbx + GS_TICK]
    mov     qword [r12 + RAID_LAST_MOVE_TICK], rax
    jmp     .tick_done

; -------------------------------------------------------
.tick_combat:
    ; tick enemies every tick
    call    tick_enemies

    ; Don't resolve until at least RAID_MOVE_INTERVAL ticks into combat
    mov     rax, [rbx + GS_TICK]
    mov     rcx, [r12 + RAID_LAST_MOVE_TICK]
    sub     rax, rcx
    cmp     rax, RAID_MOVE_INTERVAL
    jl      .tick_done

    ; check win: enemies_remaining == 0
    movzx   eax, byte [r12 + RAID_ENEMIES_REMAINING]
    test    eax, eax
    jnz     .check_loss

    ; win — set result, award rewards
    mov     byte [r12 + RAID_ACTIVE], RAID_RESULT
    mov     byte [r12 + RAID_SETTLEMENT_BREACH], 0

    ; award gold
    movzx   eax, byte [r12 + RAID_THREAT]
    cmp     eax, 5
    jle     .aw1
    mov     eax, 5
.aw1:
    lea     rcx, [rel reward_gold_tab]
    movsx   rax, dword [rcx + rax*4]
    add     [rbx + GS_RESOURCES + RES_GOLD], rax

    ; award relics (depth 3+)
    movzx   eax, byte [rbx + GS_DEPTH]
    cmp     eax, 3
    jl      .no_relics
    movzx   eax, byte [r12 + RAID_THREAT]
    cmp     eax, 5
    jle     .aw2
    mov     eax, 5
.aw2:
    lea     rcx, [rel reward_relics_tab]
    movsx   rax, dword [rcx + rax*4]
    add     [rbx + GS_RESOURCES + RES_RELICS], rax
.no_relics:

    ; fire win event
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x32
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_POSITIVE
    jmp     .tick_done

.check_loss:
    ; check if settlement was breached
    movzx   eax, byte [r12 + RAID_SETTLEMENT_BREACH]
    test    eax, eax
    jz      .tick_done

    ; loss
    mov     byte [r12 + RAID_ACTIVE], RAID_RESULT

    ; morale penalty to all dwarves
    lea     rax, [rbx + GS_DWARVES]
    mov     ecx, MAX_DWARVES
.morale_loop:
    test    ecx, ecx
    jz      .morale_done
    movzx   edx, byte [rax + DWARF_ALIVE]
    test    edx, edx
    jz      .morale_next
    movzx   edx, byte [rax + DWARF_MORALE]
    sub     edx, 20
    jns     .morale_clamp
    xor     edx, edx
.morale_clamp:
    mov     byte [rax + DWARF_MORALE], dl
.morale_next:
    add     rax, SIZEOF_DWARF
    dec     ecx
    jmp     .morale_loop
.morale_done:

    ; fire loss event
    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x31
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_NEGATIVE
    jmp     .tick_done

; -------------------------------------------------------
.tick_result:
    ; schedule next raid and return to NONE
    inc     dword [r12 + RAID_COMPLETED]
    mov     rax, [rbx + GS_TICK]
    mov     rcx, RAID_INTERVAL

    ; UPGR_OUTPOSTS (id=19) lives in tier2 at bit 36 (= 24 + (19-16)*4).
    ; Each level adds 50 ticks to RAID_INTERVAL (+50 to +250 at max).
    push    rdx
    mov     rdx, [rbx + GS_UPGR_TIER2]
    shr     rdx, 36
    and     edx, 0xF            ; outpost level 0-5
    imul    edx, 50
    add     rcx, rdx
    pop     rdx

    add     rax, rcx
    mov     [r12 + RAID_NEXT_TICK], rax
    mov     byte [r12 + RAID_ACTIVE], RAID_NONE
    jmp     .tick_done

.tick_done:
    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret