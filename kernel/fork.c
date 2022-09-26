#include "../include/kernel/sched.h"
#include "global.h"
#include "../include/kernel/memory.h"
#include "fizzstr.h"
#include "mm_types.h"
#include "vma.h"
#include "print.h"
#include "kthread.h"
/* 
 * 复制内存时，把页置为只读，当发生写异常时，再拷贝
 */

/*
if (缺页) {
    if(用户进程) {
        判断mm_struct结构体
        存在该内存就分配物理内存并映射，（不考虑内存不足的情况，没有页面置换）
        不存在时进程dump
    } else if(内核线程) {
        内核空间永不缺页
    }
} else if (保护异常) {
    // 内核态无视一切保护，不会出现这个
    if (写保护) {
        if (mm_struct->该页可写？) {
            说明为fork的写时拷贝：
                1.把该页面置为可写，防止另一个进程重新拷贝
                2.分配一个页面，并复制该页到新页面
                3.本页的虚拟地址映射到新页面的页框上
                4.释放新页面的虚拟地址
        } else {
            段错误
        }
    } else if (特权级保护) {
        段错误
    }
}
*/
// 调用exec后，共享页表数-1，如果只剩下一个，把可写页置为可写，防止以后拷贝。如果没有了就回收内存

static void copy_pt(void* old_pgdir, void* new_pgdir)
{
    memcpy(new_pgdir, old_pgdir, PG_SIZE); // 4k大小的页目录表

    *((uint32_t*)new_pgdir + 1023) = (uint32_t)get_paddr(new_pgdir) | PG_SUPERVISOR | PG_P | PG_RWE; 
    
    for (int i = 0; i < 768; i++) { // 拷贝用户态的页表，不拷贝内核态的页表
        if (*((uint32_t*)old_pgdir + i) & 0b11) { // 如果存在并可写，全部设为只读，等页保护异常时拷贝
            uint32_t* pt = (*((uint32_t*)old_pgdir + i) & 0xfffff000) + KERNEL_VADDR_START;
            for (int j = 0; j < 1024; j++) {
                if (*(pt + j) & 0b11) {
                    *(pt + j) &= ~PG_RWE;
                    atomic_inc(&paddr2pg(*(pt + j))->count); // 引用加一
                }
            }
            uint32_t* new_pt = get_free_page(KERNEL_POOL);
            memcpy(new_pt, pt, PG_SIZE);
            *((uint32_t*)new_pgdir + i) = (uint32_t)get_paddr(new_pt) | PG_P | PG_RWE | PG_USER; // 写入新页目录表
        }
    }
}

int copy_mm(uint32_t clone_flags, struct task_struct* t)
{
    if (clone_flags & CLONE_VM) { // 使用该标志就是共享
        PANIC;
        t->mm->users++;
        return 0;
    }
    struct mm_struct* old_mm = t->mm;
    struct mm_struct* mm = kmalloc(sizeof (struct mm_struct));
    memcpy(mm, old_mm, sizeof (struct mm_struct));
    dup_mmap(old_mm, mm); // 拷贝内存段描述符
    // 拷贝页目录表和页表
    mm->pg_dir = get_free_page(KERNEL_POOL);
    copy_pt(old_mm->pg_dir, mm->pg_dir);   
    t->mm = mm;
    return 0;
}
int copy_fs(uint32_t clone_flags, struct task_struct* t)
{
    if (clone_flags & CLONE_FS) { // 使用该标志就是共享
        PANIC;
    }
}

int copy_file(uint32_t clone_flags, struct task_struct* t)
{
    if (clone_flags & CLONE_FILES) { // 使用该标志就是共享
        PANIC;
    }
}

extern void intr_exit();
extern void switch_to(struct task_struct* now, struct task_struct* next);
/*
 * uint32_t stack_start, struct pt_regs* regs, uint32_t stack_size,
 *  int __user* parent_tidptr, int __user* child_tidptr
 */
int do_fork(uint32_t clone_flags) 
{
    struct task_struct* current = get_current();
    struct task_struct* new = get_free_page(KERNEL_POOL);
    memcpy(new, current, PG_SIZE);
    // 复制后进程名一样
    new->pid = alloc_pid();

    copy_mm(clone_flags, new); // TODO 失败处理
    //copy_file(clone_flags, new); // TODO 失败处理
    // copy_fs(clone_flags, new); // TODO 失败处理
    //printk("%x %x %x new:%x\n", do_fork, intr_exit, switch_to, new);
    //while(1);
    new->stack = (uint32_t) new + PG_SIZE - sizeof(struct intr_stack) - 4 * 5; // 4个寄存器和一个eip
    *((uint32_t*)(new->stack) + 4) = intr_exit; // switch to执行完直接跳到返回中断
    struct intr_stack* in_stk = (uint32_t) new + PG_SIZE - sizeof(struct intr_stack);
    in_stk->eax = 0; // 子进程返回0
    new->state = READY;
    list_push_front(&ready_list, &new->task_tag);
    schedule(); // 让子进程先执行
    return new->pid;
}

int sys_fork()
{
    return do_fork(0);
}