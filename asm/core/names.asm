; =========================================================
; asm/core/names.asm - Dwarf name table
;
; 64 names packed as null-terminated strings in .data,
; with a pointer table for O(1) lookup.
;
; Exports:
;   asm_get_dwarf_name(uint8_t idx [dil]) -> rax (char*)
;     Returns pointer to name string. idx is masked to 0-63.
;
; STACK: no pushes, no calls — alignment irrelevant.
; =========================================================

section .data

; --- Name strings ---
; Tolkien
n00: db "Durin",       0
n01: db "Balin",       0
n02: db "Dwalin",      0
n03: db "Gloin",       0
n04: db "Gimli",       0
n05: db "Thorin",      0
n06: db "Bifur",       0
n07: db "Bofur",       0
n08: db "Bombur",      0
n09: db "Dori",        0
n10: db "Nori",        0
n11: db "Ori",         0
n12: db "Fili",        0
n13: db "Kili",        0
n14: db "Oin",         0
n15: db "Dain",        0

; Warhammer
n16: db "Gotrek",      0
n17: db "Snorri",      0
n18: db "Grombrindal", 0
n19: db "Thorgrim",    0
n20: db "Ungrim",      0
n21: db "Burlok",      0
n22: db "Grimjaw",     0
n23: db "Ironbeard",   0
n24: db "Kadrin",      0
n25: db "Alaric",      0
n26: db "Borrik",      0
n27: db "Hrunting",    0

; D&D
n28: db "Bruenor",     0
n29: db "Pwent",       0
n30: db "Stokely",     0
n31: db "Meldrath",    0
n32: db "Dugmaren",    0
n33: db "Clangeddin",  0
n34: db "Haela",       0
n35: db "Moradin",     0
n36: db "Dumathoin",   0
n37: db "Vergadain",   0
n38: db "Thard",       0
n39: db "Abbathor",    0

; Divinity 2
n40: db "Beastie",     0
n41: db "Varok",       0
n42: db "Skarlag",     0
n43: db "Dallis",      0

; Deep Rock Galactic
n44: db "Karl",        0
n45: db "Bosco",       0
n46: db "Gunnar",      0
n47: db "Holt",        0

; Original
n48: db "Aldric",      0
n49: db "Bragni",      0
n50: db "Dorgrim",     0
n51: db "Embrik",      0
n52: db "Fjolnir",     0
n53: db "Grundi",      0
n54: db "Halvek",      0
n55: db "Ingvar",      0
n56: db "Jorund",      0
n57: db "Kolgrim",     0
n58: db "Lodur",       0
n59: db "Magni",       0
n60: db "Narvok",      0
n61: db "Oskar",       0
n62: db "Pelgrin",     0
n63: db "Ragnar",      0

; --- Pointer table ---
name_table:
    dq n00, n01, n02, n03, n04, n05, n06, n07
    dq n08, n09, n10, n11, n12, n13, n14, n15
    dq n16, n17, n18, n19, n20, n21, n22, n23
    dq n24, n25, n26, n27, n28, n29, n30, n31
    dq n32, n33, n34, n35, n36, n37, n38, n39
    dq n40, n41, n42, n43, n44, n45, n46, n47
    dq n48, n49, n50, n51, n52, n53, n54, n55
    dq n56, n57, n58, n59, n60, n61, n62, n63

section .text
    global asm_get_dwarf_name

; ---------------------------------------------------------
; asm_get_dwarf_name(idx=dil) -> rax (const char*)
; STACK: no pushes, no calls.
; ---------------------------------------------------------
asm_get_dwarf_name:
    movzx   rax, dil
    and     rax, 63                     ; mask to 0-63
    lea     rcx, [rel name_table]
    mov     rax, [rcx + rax * 8]        ; pointer lookup
    ret