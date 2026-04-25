BITS 64

section .text
global printString
global printInt

printString:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi

    mov rsi, rdi
    xor rdx, rdx

.printString_len_loop:
    cmp byte [rsi + rdx], 0
    je .printString_write
    inc rdx
    jmp .printString_len_loop

.printString_write:
    mov rax, 1
    mov rdi, 1
    syscall

    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rbp
    ret

printInt:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    sub rsp, 40

    mov rax, rdi
    lea rsi, [rsp + 39]
    mov byte [rsi], 0
    dec rsi

    cmp rax, 0
    jne .printInt_check_negative

    mov byte [rsi], '0'
    mov rdi, rsi
    call printString
    jmp .printInt_done

.printInt_check_negative:
    xor rbx, rbx
    cmp rax, 0
    jge .printInt_convert
    mov rbx, 1
    neg rax

.printInt_convert:
    mov rcx, 10

.printInt_loop:
    xor rdx, rdx
    div rcx
    add dl, '0'
    mov [rsi], dl
    dec rsi
    cmp rax, 0
    jne .printInt_loop

    cmp rbx, 0
    je .printInt_prepare
    mov byte [rsi], '-'
    jmp .printInt_call

.printInt_prepare:
    inc rsi

.printInt_call:
    mov rdi, rsi
    call printString

.printInt_done:
    add rsp, 40
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rbp
    ret

