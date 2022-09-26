#include "init.h"
#include "interrupt.h"
#include "../include/kernel/memory.h"
#include "../include/device/timer.h"
#include "kthread.h"
#include "console.h"
#include "tss.h"
#include "ide.h"
#include "fs.h"
#include "mm_types.h"
#include "vma.h"
#include "fizzstr.h"
#include "exec.h"
void init_all(){
    idt_init();
    intr_disable();
    memory_init();	  // 初始化内存管理系统
    thread_init();    // 初始化线程相关结构
    timer_init();     // 初始化PIT
    console_init();   // 控制台初始化最好放在开中断之前
    keyboard_init();  // 键盘初始化
    tss_init();       // tss初始化
    intr_enable();    // 后面的ide_init需要打开中断
    ide_init();	     // 初始化硬盘
    filesys_init();   // 初始化文件系统
}