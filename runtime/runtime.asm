BITS 64

section .text
global __poc_write

__poc_write:
    mov rax, 1
    syscall
    ret

