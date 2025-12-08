; resource.asm - ASMoria additional resources (PIC-ready)
; For future resources like stone, iron, etc.

global init_resources
global resource_tick
global stone
global iron

section .data
stone: dq 0
iron: dq 0

section .text

; ------------------------------
; init_resources
; Initialize all resources to 0
; ------------------------------
init_resources:
    mov qword [rel stone], 0
    mov qword [rel iron], 0
    ret

; ------------------------------
; resource_tick
; Increment resources each game tick
; ------------------------------
resource_tick:
    mov rax, [rel stone]
    add rax, 1           ; 1 stone per tick
    mov [rel stone], rax

    mov rax, [rel iron]
    add rax, 1           ; 1 iron per tick
    mov [rel iron], rax
    ret