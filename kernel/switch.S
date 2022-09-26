[bits 32]
global switch_to

section .text
switch_to:
    ; next
    ; now
    ; eip
    push esi ;C函数调用ABI
    push edi
    push ebx
    push ebp

    ; 切换栈  
    mov eax, [esp + 20]
    mov [eax], esp
    mov eax, [esp + 24]
    mov esp, [eax]

    pop ebp
    pop ebx
    pop edi
    pop esi
    ret