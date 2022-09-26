
[bits 32]
section .text

global put_char2
put_char2:
    push ebp
    mov ebp, esp
    push eax
    push edx
    push ebx
    mov ebx, [ebp + 8] ;获取参数

    ;低16位时offset
    movzx eax, bx
    shl eax, 1  ;乘2
    add eax, 0xb8000
    shr ebx, 16
    ;16-32为ascii码及颜色
    mov [eax], bx
   
    pop ebx
    pop edx
    pop eax
    pop ebp
    ret

;pushad需要18clocks 而push r只需要2clocks
global set_cursor2
set_cursor2:
    push ebp
    mov ebp, esp
    push eax
    push edx
    push ebx
    mov ebx, [ebp + 8] ;获取参数

    ;低八位
    mov al, 0x0F
    mov dx, 0x3D4
    out dx, al
    mov al, bl
    mov dx, 0x3D5
    out dx, al
        
    ;高八位
    mov al, 0x0E
    mov dx, 0x3D4
    out dx, al
    mov al, bh
    mov dx, 0x3D5
    out dx, al
   
    pop ebx
    pop edx
    pop eax
    pop ebp
    ret

;in 或 out 指令对立即数的端口进行操作时，只能是8位立即数，
;CRT寄存器组的端口地址为0x3D4 0x3D5 所以要用dx操作
global set_start_addr
set_start_addr:
    push ebp
    mov ebp, esp
    push eax
    push edx
    push ebx
    mov ebx, [ebp + 8] ;获取参数

    ;低八位
    mov al, 0x0D
    mov dx, 0x3D4
    out dx, al
    mov al, bl
    mov dx, 0x3D5
    out dx, al
        
    ;高八位
    mov al, 0x0C
    mov dx, 0x3D4
    out dx, al
    mov al, bh
    mov dx, 0x3D5
    out dx, al
   
    pop ebx
    pop edx
    pop eax
    pop ebp
    ret