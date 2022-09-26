#ifndef __KERNEL_THREAD_H
#define __KERNEL_THREAD_H
#include "sched.h"

struct task_struct* kthread_create(int (*threadfn)(void *data), void *data, int prio, const char* name);
void schedule(void);
void thread_init(void);
void thread_yield(void);
void thread_block(void);
void thread_unblock(struct task_struct* blocked);
#endif