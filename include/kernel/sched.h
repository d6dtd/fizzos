#ifndef __KERNEL_SCHED_H
#define __KERNEL_SCHED_H

#include "fizzint.h"
#include "list.h"
#include "./memory.h"
#include "file.h"
#include "dir.h"


/*
 * cloning flags:
 */
#define CSIGNAL		0x000000ff	/* signal mask to be sent at exit */
#define CLONE_VM	0x00000100	/* set if VM shared between processes */
#define CLONE_FS	0x00000200	/* set if fs info shared between processes */
#define CLONE_FILES	0x00000400	/* set if open files shared between processes */
#define CLONE_SIGHAND	0x00000800	/* set if signal handlers and blocked signals shared */
#define CLONE_PTRACE	0x00002000	/* set if we want to let tracing continue on the child too */
#define CLONE_VFORK	0x00004000	/* set if the parent wants the child to wake it up on mm_release */
#define CLONE_PARENT	0x00008000	/* set if we want to have the same parent as the cloner */
#define CLONE_THREAD	0x00010000	/* Same thread group? */
#define CLONE_NEWNS	0x00020000	/* New namespace group? */
#define CLONE_SYSVSEM	0x00040000	/* share system V SEM_UNDO semantics */
#define CLONE_SETTLS	0x00080000	/* create a new TLS for the child */
#define CLONE_PARENT_SETTID	0x00100000	/* set the TID in the parent */
#define CLONE_CHILD_CLEARTID	0x00200000	/* clear the TID in the child */
#define CLONE_DETACHED		0x00400000	/* Unused, ignored */
#define CLONE_UNTRACED		0x00800000	/* set if the tracing process can't force CLONE_PTRACE on this clone */
#define CLONE_CHILD_SETTID	0x01000000	/* set the TID in the child */
/* 0x02000000 was previously the unused CLONE_STOPPED (Start in stopped state)
   and is now available for re-use. */
#define CLONE_NEWUTS		0x04000000	/* New utsname group? */
#define CLONE_NEWIPC		0x08000000	/* New ipcs */
#define CLONE_NEWUSER		0x10000000	/* New user namespace */
#define CLONE_NEWPID		0x20000000	/* New pid namespace */
#define CLONE_NEWNET		0x40000000	/* New network namespace */
#define CLONE_IO		0x80000000	/* Clone io context */

#define STACK_MAGIC 0x12345678
#define TASK_SIZE 0xc0000000

/* 
 * 执行exec系统调用时，修改中断栈的内容，返回时就变成一个新进程了（TODO 以后再看看是不是这样）
 * 内核栈要和中断栈分开，不能改变了中断栈的内容，不然可能无法返回
 */
struct intr_stack {
    uint32_t vec_no;	 // kernel.S 宏VECTOR中push %1压入的中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;	 // 虽然pushad把esp也压入,但esp是不断变化的,所以会被popad忽略
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t error_code;
    uint32_t eip_old;
    uint32_t cs_old;
    uint32_t eflags;
    uint32_t esp_old;  // 如果没有改变特权级，则没有这两个
    uint32_t ss_old;   // 如果没有改变特权级，则没有这两个
};

/* 
 * 线程切换时压下的数据
 */
struct thread_switch_data {
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    union {
        /* 调用switch_to函数压下的数据 */
        struct {
            uint32_t eip;
            struct task_struct* now;
            struct task_struct* next;
        };
        /* 线程第一次调度时用到 */
        struct {
            void (*kthread_start)(int (*threadfn)(void *data), void *data);
            void* unused;
            int (*threadfn)(void*);
            void *data;
        };
    };       
};

enum TASK_STATE {
    RUNNING,
    READY,
    BLOCKED,
    EXIT
};

struct fs_struct {
    struct dentry* cwd;     // current working directory
    struct dentry* root;    // 
};

struct task_struct {
    void* stack;
    enum TASK_STATE state;
    pid_t pid;
    char pname[10]; // 进程名为current->fs->cwd->fname
    int exit_code;
    uint32_t prio;
    uint32_t elapsed_ticks;
    uint32_t ticks;
    struct mm_struct *mm;
    struct fs_struct* fs;
    struct files_struct* files;
    struct vaddr_pool pool;
    struct list_elem task_tag;
    uint32_t stack_magic;
};

extern struct list ready_list; 

static inline struct task_struct* get_current(void) {
    uint32_t p;
    asm volatile("movl %%esp, %0" : "=g"(p));
    return (struct task_struct*)(p & 0xfffff000);
}

void pid_init(void);
pid_t alloc_pid(void);
void free_pid(pid_t pid);
void switch_to(struct task_struct* now, struct task_struct* next);
void init_thread(struct task_struct* t, int (*threadfn)(void *data),
                    void *data, int prio, const char* name);
#endif