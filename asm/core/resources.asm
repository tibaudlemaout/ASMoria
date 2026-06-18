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

extern asm_rng_next
extern asm_tavern_buff_active

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
; clamp_storage_caps — r12=state
; Clamps gold/stone/wood/food to their storage caps.
; Cap = base + level * per_level (Vault/Warehouse/Granary)
; Clobbers: rax, rcx, rdx
; ---------------------------------------------------------
clamp_storage_caps:
    push    rax
    push    rcx
    push    rdx

    ; --- Gold cap (Vault) ---
    mov     rax, [r12 + GS_UPGR_TIER1]
    shr     rax, (UPGR_VAULT * 4)
    and     rax, 0xF
    imul    rax, CAP_GOLD_PER_LEVEL
    add     rax, CAP_GOLD_BASE
    cmp     [r12 + GS_RESOURCES + RES_GOLD], rax
    jle     .gold_ok
    mov     [r12 + GS_RESOURCES + RES_GOLD], rax
.gold_ok:

    ; --- Stone cap (Warehouse) ---
    mov     rax, [r12 + GS_UPGR_TIER1]
    shr     rax, (UPGR_WAREHOUSE * 4)
    and     rax, 0xF
    imul    rax, CAP_STONE_PER_LEVEL
    add     rax, CAP_STONE_BASE
    cmp     [r12 + GS_RESOURCES + RES_STONE], rax
    jle     .stone_ok
    mov     [r12 + GS_RESOURCES + RES_STONE], rax
.stone_ok:

    ; --- Wood cap (Warehouse) ---
    mov     rax, [r12 + GS_UPGR_TIER1]
    shr     rax, (UPGR_WAREHOUSE * 4)
    and     rax, 0xF
    imul    rax, CAP_WOOD_PER_LEVEL
    add     rax, CAP_WOOD_BASE
    cmp     [r12 + GS_RESOURCES + RES_WOOD], rax
    jle     .wood_ok
    mov     [r12 + GS_RESOURCES + RES_WOOD], rax
.wood_ok:

    ; --- Food cap (Granary) ---
    mov     rax, [r12 + GS_UPGR_TIER1]
    shr     rax, (UPGR_GRANARY * 4)
    and     rax, 0xF
    imul    rax, CAP_FOOD_PER_LEVEL
    add     rax, CAP_FOOD_BASE
    cmp     [r12 + GS_RESOURCES + RES_FOOD], rax
    jle     .food_ok
    mov     [r12 + GS_RESOURCES + RES_FOOD], rax
.food_ok:

    ; --- Mana cap: base + Mana Well level * 500 ---
    mov     rax, [r12 + GS_UPGR_TIER1]
    shr     rax, (UPGR_MANA_WELL * 4)
    and     rax, 0xF
    imul    rax, 500
    add     rax, CAP_MANA_BASE          ; cap = 100 + well_level*500
    cmp     [r12 + GS_RESOURCES + RES_MANA], rax
    jle     .mana_ok
    mov     [r12 + GS_RESOURCES + RES_MANA], rax
.mana_ok:

    ; --- Iron Ore cap: 200 + OreStorage*300 ---
    mov     rax, [r12 + GS_UPGR_TIER1]
    shr     rax, (UPGR_ORE_STORAGE * 4)
    and     rax, 0xF
    imul    rax, 300
    add     rax, 200
    cmp     [r12 + GS_RESOURCES + RES_IRON_ORE], rax
    jle     .iron_ore_ok
    mov     [r12 + GS_RESOURCES + RES_IRON_ORE], rax
.iron_ore_ok:

    ; --- Gems cap: 100 + OreStorage*150 ---
    mov     rax, [r12 + GS_UPGR_TIER1]
    shr     rax, (UPGR_ORE_STORAGE * 4)
    and     rax, 0xF
    imul    rax, 150
    add     rax, 100
    cmp     [r12 + GS_RESOURCES + RES_GEMS], rax
    jle     .gems_ok
    mov     [r12 + GS_RESOURCES + RES_GEMS], rax
.gems_ok:

    ; --- Relics cap: 50 + RelicVault*100 ---
    mov     rax, [r12 + GS_UPGR_TIER1]
    shr     rax, (UPGR_RELIC_VAULT * 4)
    and     rax, 0xF
    imul    rax, 100
    add     rax, 50
    cmp     [r12 + GS_RESOURCES + RES_RELICS], rax
    jle     .relics_ok
    mov     [r12 + GS_RESOURCES + RES_RELICS], rax
.relics_ok:

    ; --- Crystals cap: 50 + RelicVault*100 ---
    mov     rax, [r12 + GS_UPGR_TIER1]
    shr     rax, (UPGR_RELIC_VAULT * 4)
    and     rax, 0xF
    imul    rax, 100
    add     rax, 50
    cmp     [r12 + GS_RESOURCES + RES_CRYSTALS], rax
    jle     .crystals_ok
    mov     [r12 + GS_RESOURCES + RES_CRYSTALS], rax
.crystals_ok:

    ; --- Crafted caps (only if Workshop built) ---
    mov     rax, [r12 + GS_UPGR_TIER1]
    shr     rax, (UPGR_WORKSHOP * 4)
    and     rax, 0xF
    test    rax, rax
    jz      .no_crafted_caps

    ; Iron Bars cap: 100 + Forge*100
    mov     rax, [r12 + GS_UPGR_TIER1]
    shr     rax, (UPGR_FORGE * 4)
    and     rax, 0xF
    imul    rax, 100
    add     rax, 100
    cmp     [r12 + GS_RESOURCES + RES_IRON_BARS], rax
    jle     .bars_ok
    mov     [r12 + GS_RESOURCES + RES_IRON_BARS], rax
.bars_ok:

    ; Weapons/Armour/Tools cap: 20 each (fixed)
    cmp     qword [r12 + GS_RESOURCES + RES_WEAPONS], 20
    jle     .wpn_ok
    mov     qword [r12 + GS_RESOURCES + RES_WEAPONS], 20
.wpn_ok:
    cmp     qword [r12 + GS_RESOURCES + RES_ARMOUR], 20
    jle     .arm_ok
    mov     qword [r12 + GS_RESOURCES + RES_ARMOUR], 20
.arm_ok:
    cmp     qword [r12 + GS_RESOURCES + RES_TOOLS], 20
    jle     .tol_ok
    mov     qword [r12 + GS_RESOURCES + RES_TOOLS], 20
.tol_ok:

    ; Ale cap: 200 + Tavern*100 + Brewery*200
    mov     eax, [r12 + GS_TAVERN_LEVEL]   ; uint32_t -- use eax to zero-extend
    imul    rax, 100
    add     rax, 200
    mov     rcx, [r12 + GS_UPGR_TIER1]
    shr     rcx, (UPGR_BREWERY * 4)
    and     rcx, 0xF
    imul    rcx, 200
    add     rax, rcx
    cmp     [r12 + GS_RESOURCES + RES_ALE], rax
    jle     .ale_ok
    mov     [r12 + GS_RESOURCES + RES_ALE], rax
.ale_ok:

.no_crafted_caps:
    pop     rdx
    pop     rcx
    pop     rax
    ret

; ---------------------------------------------------------
; apply_tavern_yield_inline
; r12=state, rax=yield -> rax scaled by tavern buff
; BUFF_FEAST: *120/100  BUFF_DRINKING_CONTEST: *90/100
; ---------------------------------------------------------
apply_tavern_yield_inline:
    push    rcx
    push    rdx
    push    rdi
    push    rsi
    push    r8

    mov     r8, rax             ; save yield in r8

    ; check FEAST
    mov     rdi, r12
    mov     esi, BUFF_FEAST
    call    asm_tavern_buff_active
    test    eax, eax
    jz      .check_contest
    mov     rax, r8
    imul    rax, 120
    xor     rdx, rdx
    mov     rcx, 100
    div     rcx
    mov     r8, rax             ; update saved yield
    jmp     .tav_yield_done

.check_contest:
    mov     rdi, r12
    mov     esi, BUFF_DRINKING_CONTEST
    call    asm_tavern_buff_active
    test    eax, eax
    jz      .tav_yield_done
    mov     rax, r8
    imul    rax, 90
    xor     rdx, rdx
    mov     rcx, 100
    div     rcx
    mov     r8, rax

.tav_yield_done:
    mov     rax, r8             ; restore yield to rax
    pop     r8
    pop     rsi
    pop     rdi
    pop     rdx
    pop     rcx
    ret

; ---------------------------------------------------------
; apply_tool_yield_inline
; rsi=current dwarf ptr, rax=yield -> rax = yield * (100+TOOL_YIELD_BONUS) / 100
; Only applies if dwarf has EQUIP_TOOL
; Clobbers: rcx, rdx
; ---------------------------------------------------------
apply_tool_yield_inline:
    push    rcx
    push    rdx
    movzx   ecx, byte [rsi + DWARF_EQUIPMENT]
    cmp     ecx, EQUIP_TOOL
    jne     .no_tool
    ; yield * (100 + TOOL_YIELD_BONUS) / 100
    imul    rax, (100 + TOOL_YIELD_BONUS)
    xor     rdx, rdx
    mov     rcx, 100
    div     rcx
.no_tool:
    pop     rdx
    pop     rcx
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
    jz      .loop_done

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
    call    apply_tool_yield_inline
    call    apply_tavern_yield_inline
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
    call    apply_tool_yield_inline
    call    apply_tavern_yield_inline
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
    call    apply_tool_yield_inline
    call    apply_tavern_yield_inline
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
    call    apply_tool_yield_inline
    call    apply_tavern_yield_inline
    add     [r12 + GS_RESOURCES + RES_FOOD], rax
    add     [r12 + GS_PRESTIGE + PRESTIGE_RESOURCES], rax

.next:
    add     rsi, SIZEOF_DWARF
    dec     rbx
    jmp     .loop

.loop_done:
    call    clamp_storage_caps

.done:
    add     rsp, 8
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret