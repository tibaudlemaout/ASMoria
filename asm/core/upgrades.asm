; =========================================================
; asm/core/upgrades.asm - All upgrade categories
;
; IDs 0-2: tools      (gold+stone costs, max 3)
; IDs 3-4: workforce  (gold+stone costs, max 3)
; IDs 5:   watchtower (gold+stone, max 3)
; IDs 6:   rune halls (gold+stone+mana after lv1, max 5)
; IDs 7:   mana well  (gold+stone, max 3)
;
; STACK:
;   entry   :  8 mod 16
;   push rbp: 16  push rbx: 24  push r12: 32
;   push r13: 40  push r14: 48  push r15: 56
;   6 pushes, NO calls.
; =========================================================

%include "core/offsets.inc"

section .data

; Tools extended costs (levels 4-5, index 3=Lv4, 4=Lv5)
tools_ext_gold:   dd 400, 700
tools_ext_stone:  dd 200, 350
tools_ext_wood:   dd 80,  150
tools_ext_bars:   dd 5,   15

; Watch Tower extended costs (levels 4-5)
wt_ext_gold:      dd 400, 700
wt_ext_stone:     dd 300, 500
wt_ext_bars:      dd 3,   8

; Mana Well extended costs (levels 4-5)
mw_ext_gold:      dd 400, 700
mw_ext_stone:     dd 200, 350
mw_ext_gems:      dd 2,   5

; Ore Storage costs (level 1-3, iron ore cost)
ore_gold:         dd 300, 600, 1000
ore_stone:        dd 400, 800, 1500
ore_iron_ore:     dd 200, 400,  800

; Forge costs (iron bars cost)
forge_gold:       dd 500,  900, 1500
forge_stone:      dd 300,  600, 1000
forge_bars:       dd 10,   25,   50

; Brewery costs (ale cost)
brew_gold:        dd 400,  700, 1200
brew_wood:        dd 300,  600, 1000
brew_ale:         dd 100,  250,  500

; Deep Barracks costs (iron bars)
dbar_gold:        dd 800, 1500, 2500
dbar_stone:       dd 500, 1000, 1800
dbar_bars:        dd 15,   35,   70

; Relic Vault costs (relics)
rvault_gold:      dd 600, 1200, 2000
rvault_stone:     dd 800, 1600, 2800
rvault_relics:    dd 20,   50,  100

; Crystal Conduit costs (crystals)
conduit_gold:     dd 800, 1500, 2500
conduit_stone:    dd 600, 1200, 2000
conduit_crystals: dd 15,   35,   70

; Tavern cost tables (indexed by current level 0-2) (indexed by current level 0=Lv1, 1=Lv2, 2=Lv3)
tavern_gold_cost:  dd 500,  1000, 2000
tavern_stone_cost: dd 400,   800, 1500
tavern_wood_cost:  dd 200,   400,  800
tavern_bars_cost:  dd 0,       5,   15   ; 0 = no bars needed (Lv1 uses iron ore instead)

; Barracks cost tables (indexed by current level 0-2)
barracks_gold_cost:  dd 200, 400, 700
barracks_stone_cost: dd 100, 200, 350

section .text
    global asm_buy_upgrade

asm_buy_upgrade:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rbx, rdi
    movzx   r12, sil                    ; upgrade id

    cmp     r12, UPGR_COUNT
    jge     .fail

    ; --- get current level ---
    mov     rax, [rbx + GS_UPGR_TIER1]
    mov     rcx, r12
    shl     rcx, 2
    shr     rax, cl
    and     rax, 0xF
    mov     r13, rax                    ; current level

    ; --- check max level by id ---
    cmp     r12, 3
    jl      .chk_tools
    cmp     r12, 5
    jl      .chk_workforce
    cmp     r12, 6
    je      .chk_runehalls
    cmp     r12, 7
    je      .chk_manawell
    cmp     r12, UPGR_VAULT
    je      .chk_storage3
    cmp     r12, UPGR_WAREHOUSE
    je      .chk_storage3
    cmp     r12, UPGR_GRANARY
    je      .chk_storage3
    cmp     r12, UPGR_WORKSHOP
    je      .chk_workshop
    cmp     r12, UPGR_TAVERN
    je      .chk_tavern
    ; New upgrades 13-18
    cmp     r12, UPGR_ORE_STORAGE
    jae     .chk_new_upgrades
    ; id 5 = watchtower
    cmp     r13, UPGR_MAX_WATCHTOWER
    jge     .fail
    jmp     .compute_cost

.chk_storage3:
    cmp     r13, UPGR_MAX_STORAGE
    jge     .fail
    jmp     .compute_cost

.chk_workshop:
    cmp     r13, UPGR_MAX_WORKSHOP
    jge     .fail
    jmp     .compute_cost

.chk_tavern:
    cmp     r13, UPGR_MAX_TAVERN
    jge     .fail
    jmp     .compute_cost

.chk_new_upgrades:
    ; All new upgrades use generic max-3 check
    ; except deep barracks which needs depth check
    cmp     r12, UPGR_DEEP_BARRACKS
    je      .chk_deep_barracks
    cmp     r13, 3
    jge     .fail
    jmp     .compute_cost
.chk_deep_barracks:
    cmp     r13, UPGR_MAX_DEEP_BARRACKS
    jge     .fail
    jmp     .compute_cost

.chk_tools:
    cmp     r13, UPGR_MAX_TOOLS
    jge     .fail
    jmp     .compute_cost

.chk_workforce:
    cmp     r13, UPGR_MAX_WORKFORCE
    jge     .fail
    jmp     .compute_cost

.chk_runehalls:
    cmp     r13, UPGR_MAX_RUNEHALLS
    jge     .fail
    jmp     .compute_cost

.chk_manawell:
    cmp     r13, UPGR_MAX_MANAWELL
    jge     .fail

.compute_cost:
    mov     r15, r13
    inc     r15                         ; next level

    ; --- select cost base and check/deduct ---
    cmp     r12, 3
    jl      .cost_tools
    cmp     r12, 5
    jl      .cost_workforce
    cmp     r12, 6
    je      .cost_runehalls
    cmp     r12, 7
    je      .cost_manawell
    cmp     r12, UPGR_VAULT
    je      .cost_vault
    cmp     r12, UPGR_WAREHOUSE
    je      .cost_warehouse
    cmp     r12, UPGR_GRANARY
    je      .cost_granary
    cmp     r12, UPGR_WORKSHOP
    je      .cost_workshop
    cmp     r12, UPGR_TAVERN
    je      .cost_tavern
    cmp     r12, UPGR_ORE_STORAGE
    je      .cost_ore_storage
    cmp     r12, UPGR_FORGE
    je      .cost_forge
    cmp     r12, UPGR_BREWERY
    je      .cost_brewery
    cmp     r12, UPGR_DEEP_BARRACKS
    je      .cost_deep_barracks
    cmp     r12, UPGR_RELIC_VAULT
    je      .cost_relic_vault
    cmp     r12, UPGR_CRYSTAL_CONDUIT
    je      .cost_crystal_conduit
    ; id 5 = watchtower
    jmp     .cost_watchtower

.cost_tools:
    ; Levels 4-5 use ext tables with iron bars
    cmp     r13d, 3
    jge     .cost_tools_ext
    mov     r14, UPGR_COST_GOLD_TOOLS
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_TOOLS
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    mov     rax, UPGR_COST_WOOD_TOOLS
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_WOOD], rax
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_TOOLS
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    mov     rax, UPGR_COST_WOOD_TOOLS
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_WOOD],  rax
    jmp     .write_level

.cost_workforce:
    ; Barracks (id=3): exponential cost 200/400/700
    ; Recruiters (id=4): linear cost
    cmp     r12, UPGR_BARRACKS
    jne     .cost_workforce_linear

    ; Barracks exponential gold cost table
    lea     rax, [rel barracks_gold_cost]
    mov     r14d, [rax + r13*4]         ; r13 = current level (0-based next)
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    lea     rax, [rel barracks_stone_cost]
    mov     ecx, [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_STONE], rcx
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    lea     rax, [rel barracks_stone_cost]
    movsx   rcx, dword [rax + r13*4]
    sub     [rbx + GS_RESOURCES + RES_STONE], rcx
    jmp     .write_level

.cost_workforce_linear:
    mov     r14, UPGR_COST_GOLD_WORK
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_WORK
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_WORK
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    jmp     .write_level

.cost_watchtower:
    cmp     r13d, 3
    jge     .cost_wt_ext
    mov     r14, UPGR_COST_GOLD_WATCH
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_WATCH
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_WATCH
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    jmp     .write_level

.cost_wt_ext:
    mov     rax, r13
    sub     rax, 3
    lea     rcx, [rel wt_ext_gold]
    mov     r14d, [rcx + rax*4]
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    lea     rcx, [rel wt_ext_stone]
    movsx   rcx, dword [rcx + rax*4]
    cmp     [rbx + GS_RESOURCES + RES_STONE], rcx
    jl      .fail
    lea     rcx, [rel wt_ext_bars]
    movsx   rcx, dword [rcx + rax*4]
    cmp     qword [rbx + GS_RESOURCES + RES_IRON_BARS], rcx
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD], r14
    mov     rax, r13
    sub     rax, 3
    lea     rcx, [rel wt_ext_stone]
    movsx   rcx, dword [rcx + rax*4]
    sub     [rbx + GS_RESOURCES + RES_STONE], rcx
    lea     rcx, [rel wt_ext_bars]
    movsx   rcx, dword [rcx + rax*4]
    sub     qword [rbx + GS_RESOURCES + RES_IRON_BARS], rcx
    jmp     .write_level

.cost_runehalls:
    mov     r14, UPGR_COST_GOLD_RUNE
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_RUNE
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    ; mana cost from level 2 onwards
    cmp     r15, 2
    jl      .rune_no_mana
    mov     rax, UPGR_COST_MANA_RUNE
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_MANA], rax
    jl      .fail
    mov     rax, UPGR_COST_MANA_RUNE
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_MANA], rax
.rune_no_mana:
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_RUNE
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    jmp     .write_level

.cost_manawell:
    cmp     r13d, 3
    jge     .cost_mw_ext
    mov     r14, UPGR_COST_GOLD_MANA
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_MANA
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_MANA
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    jmp     .write_level

.cost_mw_ext:
    mov     rax, r13
    sub     rax, 3
    lea     rcx, [rel mw_ext_gold]
    mov     r14d, [rcx + rax*4]
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    lea     rcx, [rel mw_ext_stone]
    movsx   rcx, dword [rcx + rax*4]
    cmp     [rbx + GS_RESOURCES + RES_STONE], rcx
    jl      .fail
    lea     rcx, [rel mw_ext_gems]
    movsx   rcx, dword [rcx + rax*4]
    cmp     qword [rbx + GS_RESOURCES + RES_GEMS], rcx
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD], r14
    mov     rax, r13
    sub     rax, 3
    lea     rcx, [rel mw_ext_stone]
    movsx   rcx, dword [rcx + rax*4]
    sub     [rbx + GS_RESOURCES + RES_STONE], rcx
    lea     rcx, [rel mw_ext_gems]
    movsx   rcx, dword [rcx + rax*4]
    sub     qword [rbx + GS_RESOURCES + RES_GEMS], rcx
    jmp     .write_level

.cost_vault:
    mov     r14, UPGR_COST_GOLD_VAULT
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_VAULT
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_VAULT
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    jmp     .write_level

.cost_warehouse:
    mov     r14, UPGR_COST_GOLD_WAREHOUSE
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_WAREHOUSE
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    mov     rax, UPGR_COST_WOOD_WAREHOUSE
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_WOOD], rax
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_WAREHOUSE
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    mov     rax, UPGR_COST_WOOD_WAREHOUSE
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_WOOD],  rax
    jmp     .write_level

.cost_granary:
    mov     r14, UPGR_COST_GOLD_GRANARY
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_GRANARY
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    mov     rax, UPGR_COST_FOOD_GRANARY
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_FOOD], rax
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_GRANARY
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    mov     rax, UPGR_COST_FOOD_GRANARY
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_FOOD],  rax
    jmp     .write_level

.cost_workshop:
    mov     r14, UPGR_COST_GOLD_WORKSHOP
    imul    r14, r15
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    mov     rax, UPGR_COST_STONE_WORKSHOP
    imul    rax, r15
    cmp     [rbx + GS_RESOURCES + RES_STONE], rax
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    mov     rax, UPGR_COST_STONE_WORKSHOP
    imul    rax, r15
    sub     [rbx + GS_RESOURCES + RES_STONE], rax
    jmp     .write_level

.cost_tavern:
    ; Tavern costs vary per level
    ; r13 = current level index (0=buying Lv1, 1=buying Lv2, 2=buying Lv3)
    ; Load per-level costs — sign-extend to 64-bit for comparison with int64 resources
    lea     rax, [rel tavern_gold_cost]
    movsx   r14, dword [rax + r13*4]    ; r14 = gold cost (64-bit)
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail

    lea     rax, [rel tavern_stone_cost]
    movsx   rcx, dword [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_STONE], rcx
    jl      .fail

    lea     rax, [rel tavern_wood_cost]
    movsx   rcx, dword [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_WOOD], rcx
    jl      .fail

    ; Iron ore check (level 1 only)
    cmp     r13d, 0
    jne     .tavern_bars_check
    cmp     qword [rbx + GS_RESOURCES + RES_IRON_ORE], 100
    jl      .fail
    sub     qword [rbx + GS_RESOURCES + RES_IRON_ORE], 100
    jmp     .tavern_deduct
.tavern_bars_check:
    ; Iron bars check (levels 2 and 3)
    lea     rax, [rel tavern_bars_cost]
    mov     ecx, [rax + r13*4]
    test    ecx, ecx
    jz      .tavern_deduct
    movsx   rcx, ecx
    cmp     qword [rbx + GS_RESOURCES + RES_IRON_BARS], rcx
    jl      .fail
    sub     qword [rbx + GS_RESOURCES + RES_IRON_BARS], rcx

.tavern_deduct:
    sub     [rbx + GS_RESOURCES + RES_GOLD],  r14
    lea     rax, [rel tavern_stone_cost]
    movsx   rcx, dword [rax + r13*4]
    sub     [rbx + GS_RESOURCES + RES_STONE], rcx
    lea     rax, [rel tavern_wood_cost]
    movsx   rcx, dword [rax + r13*4]
    sub     [rbx + GS_RESOURCES + RES_WOOD],  rcx

    ; Also update tavern_level in GameState directly
    mov     eax, r13d
    inc     eax
    mov     [rbx + GS_TAVERN_LEVEL], eax

    jmp     .write_level

; -------------------------------------------------------
; Extended tool costs (levels 4-5, r13=3 or 4)
; -------------------------------------------------------
.cost_tools_ext:
    mov     rax, r13
    sub     rax, 3                      ; 0=Lv4, 1=Lv5
    lea     rcx, [rel tools_ext_gold]
    mov     r14d, [rcx + rax*4]
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    lea     rcx, [rel tools_ext_stone]
    movsx   rcx, dword [rcx + rax*4]
    cmp     [rbx + GS_RESOURCES + RES_STONE], rcx
    jl      .fail
    lea     rcx, [rel tools_ext_wood]
    movsx   rcx, dword [rcx + rax*4]
    cmp     [rbx + GS_RESOURCES + RES_WOOD], rcx
    jl      .fail
    lea     rcx, [rel tools_ext_bars]
    movsx   rcx, dword [rcx + rax*4]
    cmp     qword [rbx + GS_RESOURCES + RES_IRON_BARS], rcx
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD], r14
    lea     rcx, [rel tools_ext_stone]
    mov     rax, r13
    sub     rax, 3
    movsx   rcx, dword [rcx + rax*4]
    sub     [rbx + GS_RESOURCES + RES_STONE], rcx
    lea     rcx, [rel tools_ext_wood]
    movsx   rcx, dword [rcx + rax*4]
    sub     [rbx + GS_RESOURCES + RES_WOOD], rcx
    lea     rcx, [rel tools_ext_bars]
    movsx   rcx, dword [rcx + rax*4]
    sub     qword [rbx + GS_RESOURCES + RES_IRON_BARS], rcx
    jmp     .write_level

; -------------------------------------------------------
; New upgrade cost handlers
; -------------------------------------------------------
%macro NEW_UPGR_COST 5  ; label, gold_table, stone_table, extra_res, extra_table
.cost_%1:
    lea     rax, [rel %2]
    mov     r14d, [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    lea     rax, [rel %3]
    movsx   rcx, dword [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + %4], rcx
    jl      .fail
    lea     rax, [rel %5]
    movsx   rcx, dword [rax + r13*4]
    cmp     qword [rbx + GS_RESOURCES + RES_GOLD], rcx   ; placeholder check
    sub     [rbx + GS_RESOURCES + RES_GOLD], r14
    lea     rax, [rel %3]
    movsx   rcx, dword [rax + r13*4]
    sub     [rbx + GS_RESOURCES + %4], rcx
    lea     rax, [rel %5]
    movsx   rcx, dword [rax + r13*4]
    sub     qword [rbx + GS_RESOURCES + RES_GOLD], rcx
    jmp     .write_level
%endmacro

; Ore Storage: gold + stone + iron ore
.cost_ore_storage:
    lea     rax, [rel ore_gold]
    mov     r14d, [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    lea     rax, [rel ore_stone]
    movsx   rcx, dword [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_STONE], rcx
    jl      .fail
    lea     rax, [rel ore_iron_ore]
    movsx   rcx, dword [rax + r13*4]
    cmp     qword [rbx + GS_RESOURCES + RES_IRON_ORE], rcx
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD], r14
    lea     rax, [rel ore_stone]
    movsx   rcx, dword [rax + r13*4]
    sub     [rbx + GS_RESOURCES + RES_STONE], rcx
    lea     rax, [rel ore_iron_ore]
    movsx   rcx, dword [rax + r13*4]
    sub     qword [rbx + GS_RESOURCES + RES_IRON_ORE], rcx
    jmp     .write_level

; Forge: gold + stone + iron bars
.cost_forge:
    lea     rax, [rel forge_gold]
    mov     r14d, [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    lea     rax, [rel forge_stone]
    movsx   rcx, dword [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_STONE], rcx
    jl      .fail
    lea     rax, [rel forge_bars]
    movsx   rcx, dword [rax + r13*4]
    cmp     qword [rbx + GS_RESOURCES + RES_IRON_BARS], rcx
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD], r14
    lea     rax, [rel forge_stone]
    movsx   rcx, dword [rax + r13*4]
    sub     [rbx + GS_RESOURCES + RES_STONE], rcx
    lea     rax, [rel forge_bars]
    movsx   rcx, dword [rax + r13*4]
    sub     qword [rbx + GS_RESOURCES + RES_IRON_BARS], rcx
    jmp     .write_level

; Brewery: gold + wood + ale
.cost_brewery:
    lea     rax, [rel brew_gold]
    mov     r14d, [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    lea     rax, [rel brew_wood]
    movsx   rcx, dword [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_WOOD], rcx
    jl      .fail
    lea     rax, [rel brew_ale]
    movsx   rcx, dword [rax + r13*4]
    cmp     qword [rbx + GS_RESOURCES + RES_ALE], rcx
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD], r14
    lea     rax, [rel brew_wood]
    movsx   rcx, dword [rax + r13*4]
    sub     [rbx + GS_RESOURCES + RES_WOOD], rcx
    lea     rax, [rel brew_ale]
    movsx   rcx, dword [rax + r13*4]
    sub     qword [rbx + GS_RESOURCES + RES_ALE], rcx
    jmp     .write_level

; Deep Barracks: gold + stone + iron bars
.cost_deep_barracks:
    lea     rax, [rel dbar_gold]
    mov     r14d, [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    lea     rax, [rel dbar_stone]
    movsx   rcx, dword [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_STONE], rcx
    jl      .fail
    lea     rax, [rel dbar_bars]
    movsx   rcx, dword [rax + r13*4]
    cmp     qword [rbx + GS_RESOURCES + RES_IRON_BARS], rcx
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD], r14
    lea     rax, [rel dbar_stone]
    movsx   rcx, dword [rax + r13*4]
    sub     [rbx + GS_RESOURCES + RES_STONE], rcx
    lea     rax, [rel dbar_bars]
    movsx   rcx, dword [rax + r13*4]
    sub     qword [rbx + GS_RESOURCES + RES_IRON_BARS], rcx
    jmp     .write_level

; Relic Vault: gold + stone + relics
.cost_relic_vault:
    lea     rax, [rel rvault_gold]
    mov     r14d, [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    lea     rax, [rel rvault_stone]
    movsx   rcx, dword [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_STONE], rcx
    jl      .fail
    lea     rax, [rel rvault_relics]
    movsx   rcx, dword [rax + r13*4]
    cmp     qword [rbx + GS_RESOURCES + RES_RELICS], rcx
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD], r14
    lea     rax, [rel rvault_stone]
    movsx   rcx, dword [rax + r13*4]
    sub     [rbx + GS_RESOURCES + RES_STONE], rcx
    lea     rax, [rel rvault_relics]
    movsx   rcx, dword [rax + r13*4]
    sub     qword [rbx + GS_RESOURCES + RES_RELICS], rcx
    jmp     .write_level

; Crystal Conduit: gold + stone + crystals
.cost_crystal_conduit:
    lea     rax, [rel conduit_gold]
    mov     r14d, [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_GOLD], r14
    jl      .fail
    lea     rax, [rel conduit_stone]
    movsx   rcx, dword [rax + r13*4]
    cmp     [rbx + GS_RESOURCES + RES_STONE], rcx
    jl      .fail
    lea     rax, [rel conduit_crystals]
    movsx   rcx, dword [rax + r13*4]
    cmp     qword [rbx + GS_RESOURCES + RES_CRYSTALS], rcx
    jl      .fail
    sub     [rbx + GS_RESOURCES + RES_GOLD], r14
    lea     rax, [rel conduit_stone]
    movsx   rcx, dword [rax + r13*4]
    sub     [rbx + GS_RESOURCES + RES_STONE], rcx
    lea     rax, [rel conduit_crystals]
    movsx   rcx, dword [rax + r13*4]
    sub     qword [rbx + GS_RESOURCES + RES_CRYSTALS], rcx
    jmp     .write_level

.write_level:
    mov     rcx, r12
    shl     rcx, 2
    mov     rax, 0xF
    shl     rax, cl
    not     rax
    and     [rbx + GS_UPGR_TIER1], rax
    mov     rax, r15
    shl     rax, cl
    or      [rbx + GS_UPGR_TIER1], rax

    mov     byte [rbx + GS_PENDING + PENDING_CODE],     0x41
    mov     byte [rbx + GS_PENDING + PENDING_SEVERITY], EVT_MILESTONE
    mov     byte [rbx + GS_PENDING + PENDING_DWARF],    0xFF

    mov     rax, r15
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