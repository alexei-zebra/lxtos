global _start
extern main

section .text
_start:
    xor  rbp, rbp
    pop  rdi          ; argc — лежит на вершине стека
    mov  rsi, rsp     ; argv — указатель на массив указателей
    and  rsp, ~0xF
    call main
    mov  rdi, rax
    mov  rax, 3
    int  0x80
    ud2