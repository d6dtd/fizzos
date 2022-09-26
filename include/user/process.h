#ifndef __PROCESS_H
#define __PROCESS_H
#include "../kernel/sched.h"
#define USER_STACK3_VADDR  (0xc0000000 - 0x1000)
#define USER_VADDR_START 0x8048000
void process_activate(struct task_struct *p);
void process_execute(void* filename, char* name);
#endif