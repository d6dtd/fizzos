[bits 32]
global page_fault_handler
extern page_not_present
extern page_protect_fault

page_fault_handler:
    push ds
    push es
    push fs
    push gs
    pushad

    ; 如果是从片上进入的中断,除了往从片上发送EOI外,还要往主片上发送EOI 
    mov al,0x20                   ; 中断结束命令EOI
    out 0xa0,al                   ; 向从片发送
    out 0x20,al                   ; 向主片发送

    mov eax, [esp + 48] ;取错误码
    mov ebx, cr2    ;取异常地址
    push ebx
    test eax, $1    ;errno 最后一位为0时是缺页，为1时是保护异常 还有USER位和RW位，不处理 最后三位是(U/S)(R/W)P
    jne  page2
page1:
    call page_not_present
    jmp page_done
page2:
    call page_protect_fault
page_done:
    add esp, 4
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4  ; 跳过error_code
    iretd