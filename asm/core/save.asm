; =========================================================
; asm/core/save.asm - Save/load via raw Linux syscalls
;
; No libc. All file I/O uses syscall directly.
; Scratch buffers live on the stack (no .bss) to avoid
; PIE relocation issues. .data reference uses [rel label].
;
; Syscalls: read=0, write=1, open=2, close=3
; Open flags: O_RDONLY=0, O_WRONLY|O_CREAT|O_TRUNC=577
; =========================================================

%include "core/offsets.inc"

%define SYS_READ    0
%define SYS_WRITE   1
%define SYS_OPEN    2
%define SYS_CLOSE   3

%define O_RDONLY    0
%define O_WCT       577         ; O_WRONLY|O_CREAT|O_TRUNC
%define FILE_MODE   420         ; 0644

%define SAVE_MAGIC      0x41534D52
%define SAVE_VERSION    3
%define SAVE_HDR_SIZE   16
%define CHECKSUM_SIZE   8
%define GS_SIZE         5216

%define SAVE_OK          0
%define SAVE_ERR_OPEN    1
%define SAVE_ERR_WRITE   2
%define SAVE_ERR_READ    3
%define SAVE_ERR_MAGIC   4
%define SAVE_ERR_VERSION 5
%define SAVE_ERR_SIZE    6
%define SAVE_ERR_CHKSUM  7
%define SAVE_ERR_NOFILE  8
%define ENOENT           2

section .data
; Header template — gs_size field at offset 8
save_header:
    dd SAVE_MAGIC
    dd SAVE_VERSION
    dq GS_SIZE

section .text
    global asm_state_checksum
    global asm_save_game
    global asm_load_game

; ---------------------------------------------------------
; asm_state_checksum(data=rdi, len=rsi) -> rax  (FNV-1a)
; STACK: no pushes, no calls.
; ---------------------------------------------------------
%define FNV_OFFSET  14695981039346656037
%define FNV_PRIME   1099511628211

asm_state_checksum:
    mov     rax, FNV_OFFSET
    test    rsi, rsi
    jz      .done
.loop:
    movzx   rcx, byte [rdi]
    xor     rax, rcx
    mov     rcx, FNV_PRIME
    imul    rax, rcx
    inc     rdi
    dec     rsi
    jnz     .loop
.done:
    ret

; ---------------------------------------------------------
; do_close / do_write / do_read  — inline syscall helpers
; No stack frame. Clobber rax, rcx, r11 (syscall ABI).
; ---------------------------------------------------------
do_close:
    mov     rax, SYS_CLOSE
    syscall
    ret

do_write:
    mov     rax, SYS_WRITE
    syscall
    ret

do_read:
    mov     rax, SYS_READ
    syscall
    ret

; ---------------------------------------------------------
; asm_save_game(path=rdi, state=rsi) -> rax
;
; Stack layout (after prologue, before sub rsp):
;   entry   :  8 mod 16
;   push rbp: 16  aligned
;   push rbx: 24
;   push r12: 32  aligned
;   push r13: 40
;   push r14: 48  aligned
;   push r15: 56
;   sub rsp,24: 80  aligned  <- locals + alignment
;
; Locals on stack (rsp relative):
;   [rsp+0]  : checksum scratch (8 bytes)
;   [rsp+8]  : padding
;   [rsp+16] : padding
; ---------------------------------------------------------
asm_save_game:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 24

    mov     r12, rdi                    ; path
    mov     r13, rsi                    ; state

    ; open for write
    mov     rdi, r12
    mov     rsi, O_WCT
    mov     rdx, FILE_MODE
    mov     rax, SYS_OPEN
    syscall
    test    rax, rax
    js      .err_open
    mov     r14, rax                    ; fd

    ; write header via RIP-relative ref
    mov     rdi, r14
    lea     rsi, [rel save_header]
    mov     rdx, SAVE_HDR_SIZE
    call    do_write
    cmp     rax, SAVE_HDR_SIZE
    jne     .err_write

    ; write GameState
    mov     rdi, r14
    mov     rsi, r13
    mov     rdx, GS_SIZE
    call    do_write
    cmp     rax, GS_SIZE
    jne     .err_write

    ; compute checksum, store on stack, write it
    mov     rdi, r13
    mov     rsi, GS_SIZE
    call    asm_state_checksum
    mov     [rsp], rax                  ; checksum on stack

    mov     rdi, r14
    lea     rsi, [rsp]                  ; pointer to checksum
    mov     rdx, CHECKSUM_SIZE
    call    do_write
    cmp     rax, CHECKSUM_SIZE
    jne     .err_write

    mov     rdi, r14
    call    do_close
    mov     rax, SAVE_OK
    jmp     .done

.err_open:
    neg     rax
    cmp     rax, ENOENT
    je      .ret_nofile
    mov     rax, SAVE_ERR_OPEN
    jmp     .done
.ret_nofile:
    mov     rax, SAVE_ERR_NOFILE
    jmp     .done
.err_write:
    mov     rdi, r14
    call    do_close
    mov     rax, SAVE_ERR_WRITE

.done:
    add     rsp, 24
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

; ---------------------------------------------------------
; asm_load_game(path=rdi, state=rsi) -> rax
;
; Stack layout:
;   same prologue as asm_save_game
;
; Locals on stack:
;   [rsp+0]  : header buffer  (16 bytes)
;   [rsp+16] : checksum buf   (8 bytes)
;   total local: 24 bytes -> sub rsp,24 keeps alignment
; ---------------------------------------------------------
asm_load_game:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 24

    mov     r12, rdi                    ; path
    mov     r13, rsi                    ; state

    ; open for read
    mov     rdi, r12
    mov     rsi, O_RDONLY
    mov     rdx, 0
    mov     rax, SYS_OPEN
    syscall
    test    rax, rax
    js      .err_open
    mov     r14, rax                    ; fd

    ; read header into stack buffer [rsp+0..15]
    mov     rdi, r14
    lea     rsi, [rsp]
    mov     rdx, SAVE_HDR_SIZE
    call    do_read
    cmp     rax, SAVE_HDR_SIZE
    jne     .err_read

    ; validate magic
    mov     eax, dword [rsp]
    cmp     eax, SAVE_MAGIC
    jne     .err_magic

    ; validate version
    mov     eax, dword [rsp + 4]
    cmp     eax, SAVE_VERSION
    jne     .err_version

    ; validate gs_size
    mov     rax, qword [rsp + 8]
    cmp     rax, GS_SIZE
    jne     .err_size

    ; read GameState directly into caller's buffer
    mov     rdi, r14
    mov     rsi, r13
    mov     rdx, GS_SIZE
    call    do_read
    cmp     rax, GS_SIZE
    jne     .err_read

    ; read stored checksum into [rsp+16]
    mov     rdi, r14
    lea     rsi, [rsp + 16]
    mov     rdx, CHECKSUM_SIZE
    call    do_read
    cmp     rax, CHECKSUM_SIZE
    jne     .err_read

    ; compute expected checksum
    mov     rdi, r13
    mov     rsi, GS_SIZE
    call    asm_state_checksum

    ; compare with stored
    mov     r15, qword [rsp + 16]
    cmp     rax, r15
    jne     .err_checksum

    mov     rdi, r14
    call    do_close
    mov     rax, SAVE_OK
    jmp     .done

.err_open:
    neg     rax
    cmp     rax, ENOENT
    je      .ret_nofile
    mov     rax, SAVE_ERR_OPEN
    jmp     .done
.ret_nofile:
    mov     rax, SAVE_ERR_NOFILE
    jmp     .done
.err_read:
    mov     rdi, r14
    call    do_close
    mov     rax, SAVE_ERR_READ
    jmp     .done
.err_magic:
    mov     rdi, r14
    call    do_close
    mov     rax, SAVE_ERR_MAGIC
    jmp     .done
.err_version:
    mov     rdi, r14
    call    do_close
    mov     rax, SAVE_ERR_VERSION
    jmp     .done
.err_size:
    mov     rdi, r14
    call    do_close
    mov     rax, SAVE_ERR_SIZE
    jmp     .done
.err_checksum:
    mov     rdi, r14
    call    do_close
    mov     rax, SAVE_ERR_CHKSUM

.done:
    add     rsp, 24
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret