[bits 32]
extern main
extern exit
global _start

_start:
    pop esi ; argc
    mov ecx, esp ;argv
    
    ; 以后可能还需要初始化其他东西
    push ecx
    push esi
    call main
    push eax ; 返回值在eax
    call exit