; resource.asm - ASMoria additional resources (PIC-ready)
; For future resources like stone, iron, etc.

global init_resources

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
