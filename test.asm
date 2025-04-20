; Assembly Program

section .data
    var1 dq 0
    var2 dq 0

section .bss
    intBuffer resb 20

section .text
    global _start

end:
    mov rax, 60
    xor rdi, rdi
    syscall

strlen:
    xor rcx, rcx
.next:
    cmp byte [rsi + rcx], 0
    je .done
    inc rcx
    jmp .next
.done:
    mov rdx, rcx
    ret

print:
    push rcx
    mov rsi, rdi
    mov rax, 1
    mov rdi, 1
    call strlen
    syscall
    pop rcx
    ret

printInt:
    mov rcx, intBuffer + 19
    mov rax, rsi
    mov rbx, 10
    cmp rax, 0
    jne .convert
    mov byte [rcx], '0'
    dec rcx
    jmp .done
.convert:
.loop:
    xor rdx, rdx
    div rbx
    add dl, '0'
    dec rcx
    mov [rcx], dl
    cmp rax, 0
    jne .loop
.done:
    mov rdi, 1
    mov rax, 1
    mov rsi, rcx
    mov rdx, intBuffer + 19
    sub rdx, rcx
    syscall
    ret

_start:
    push 2
    push 4
    pop rbx
    pop rax
    add rax, rbx
    push rax
    push 14
    pop rbx
    pop rax
    imul rax, rbx
    mov [var1], rax

    push 2
    push 4
    pop rbx
    pop rax
    add rax, rbx
    push rax
    push 18
    pop rbx
    pop rax
    imul rax, rbx
    mov [var2], rax

    mov rsi, [var1]
    call printInt
    call end
