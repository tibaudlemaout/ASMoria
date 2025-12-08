; core.asm - ASMoria ASM core library (PIC-ready)
; Target: x86_64 NASM syntax
; This file contains the main tick function and resource storage

global asm_tick  ; Make tick function accessible to C
global gold      ; Make gold variable accessible to C

section .data
; Resource storage (RIP-relative for PIC)
gold: dq 0        ; 64-bit integer to store current gold

section .text

; ------------------------------
; asm_tick
; Increments gold by 1 per call
; Called from C layer every game tick
; ------------------------------
asm_tick:
    mov rax, [rel gold]   ; Load current gold into RAX (PIC-safe)
    add rax, 1            ; Increment gold by 1
    mov [rel gold], rax   ; Store updated gold back
    ret
