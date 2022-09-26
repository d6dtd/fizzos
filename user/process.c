#include "process.h"
#include "../include/kernel/memory.h"
#include "fizzstr.h"
#include "bitmap.h"
#include "../include/kernel/sched.h"
#include "print.h"
#include "tss.h"
#include "mm_types.h"
#include "exec.h"
#include "kthread.h"
#define MAX_PID 65536

/* static struct task_struct *copy_process(unsigned long clone_flags,
					unsigned long stack_start,
					struct pt_regs *regs,
					unsigned long stack_size,
					int __user *child_tidptr,
					struct pid *pid,
					int trace) 
*/

static struct bitmap pid_bitmap;

void pid_init(void) {
	PRINTK("pid_init begin\n");
	pid_bitmap.bits = kmalloc(DIV_ROUND_UP(MAX_PID, 8));
	PRINTK("pid_init done\n");
	bitmap_init(&pid_bitmap, MAX_PID);
}

pid_t alloc_pid(void) {
	return bitmap_scan_set(&pid_bitmap);
}
void free_pid(pid_t pid) {
	bitmap_reset(&pid_bitmap, pid);
}

extern void intr_exit(void);

void kthread_start(int (*threadfn)(void *data), void *data);

static void mm_struct_init(struct task_struct* t)
{
    struct mm_struct* mm = kzalloc(sizeof (struct mm_struct));
    mm->pg_dir = NULL;
    mm->users = 1;
}


void page_dir_activate(struct task_struct* p_thread) {
	uint32_t pagedir_paddr = 0x100000;

	if (p_thread->mm != NULL) {
		pagedir_paddr = get_paddr(p_thread->mm->pg_dir);
	}

	asm volatile ("movl %0, %%cr3" : : "r" (pagedir_paddr) : "memory");
}

void process_activate(struct task_struct *p) {
	ASSERT(p != NULL);
	page_dir_activate(p);

	if (p->mm) {
		update_esp0(p);
	}
}

int init_proc(void* arg)
{
	asm volatile("subl %0, %%esp"::"r"(sizeof(struct intr_stack))); // 先预留一下空间，不然等下回不来了
	struct task_struct* current = get_current();
	intr_disable();
	PRINTK("addr:%x %d\n", init_proc, sizeof(struct intr_stack));
    int ret = do_exec("/init", NULL, NULL);
	update_esp0(current); // 更新一下，不然无法进入缺页中断
	page_dir_activate(current);
    if (ret == 0) {
      asm volatile("movl %0, %%esp"
                  :
                  : "g"((uint32_t)current + PG_SIZE - sizeof(struct intr_stack)));
				//  while(1);
      asm volatile("jmp intr_exit"
                  :
                  :
                  : "memory");
    }
	PRINTK("init process failed\n");
	while(1);
    return ret;
}
void make_init_proc()
{
  kthread_create(init_proc, NULL, 10, "init");
}