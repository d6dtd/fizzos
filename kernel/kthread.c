#include "kthread.h"
#include "../include/kernel/memory.h"
#include "global.h"
#include "interrupt.h"
#include "fizzstr.h"
#include "print.h"
#include "process.h"
#include "file.h"
#include "../include/kernel/atomic.h"
#include "console.h"
#include "mm_types.h"

struct list ready_list; // 线程的就绪队列

struct task_struct* idle;

void idle_thread(void) {
    while (1) {
        thread_block(); // 第一次运行时卡在这里，状态设为阻塞，下次运行时就直接执行下面的指令了
        /* 
         * 使程序停止运行，处理器进入暂停状态，不执行任何操作，不影响标志。当RESET线上有复位信号
         * 、CPU响应非屏蔽中断、CPU响应可屏蔽中断3种情况之一时，CPU脱离暂停状态，执行HLT的下一条
         * 指令。如果hlt指令之前，做了cli，那可屏蔽中断不能唤醒cpu。
         * 所以hlt之前先sti，不然无法调度了
         */
        asm volatile("sti; hlt" : : : "memory");
    }
}

void thread_block() {
    enum intr_status old = intr_disable();
    struct task_struct* current = get_current();
    current->state = BLOCKED;
    schedule();
    intr_set_status(old);
}

void thread_unblock(struct task_struct* blocked) {
    enum intr_status old = intr_disable();
    blocked->state = READY;
    list_push_back(&ready_list, &blocked->task_tag);
    intr_set_status(old);
}

void kthread_start(int (*threadfn)(void *data), void *data) {
    /* 
     * 中断处理函数返回时会恢复中断，但是新线程第一次中断时不返回，
     * 所以要开中断才能继续调度(时钟中断)，以后执行时会返回到switch_to函数，
     * 然后中断处理函数返回后自动开启中断
     */
    intr_enable(); 
    threadfn(data);
}

static void files_struct_init(struct task_struct* t)
{
    struct files_struct* files = kzalloc(sizeof (struct files_struct));
    atomic_inc(&file_table[0].count);
    atomic_inc(&file_table[1].count);
    atomic_inc(&file_table[2].count);
    files->fd_array[0] = &file_table[0];
    files->fd_array[1] = &file_table[1];
    files->fd_array[2] = &file_table[2];
    files->open_fds = 0b111;
    files->close_on_exec = 0;
    files->map_file = NULL;
    t->files = files;
}

static void fs_struct_init(struct task_struct* t)
{
    struct fs_struct* fs = kzalloc(sizeof (struct fs_struct));
    fs->root = &root_dir;
    fs->cwd = &root_dir;
    t->fs = fs;
}


void init_thread(struct task_struct* t, int (*threadfn)(void *data),
                    void *data, int prio, const char* name)
{
    t->stack_magic = STACK_MAGIC;
    /* 伪装成进入switch_to函数后的样子 */
    t->stack = (uint32_t)t + PG_SIZE - sizeof (struct intr_stack) - sizeof (struct thread_switch_data);
    t->state = READY;
    strcpy(t->pname, name);
    t->prio = prio;
    t->pid = alloc_pid();
    t->mm = NULL;
    files_struct_init(t);
    fs_struct_init(t);
    struct thread_switch_data* switch_data = t->stack;
    switch_data->kthread_start = kthread_start;
    switch_data->threadfn = threadfn;
    switch_data->data = data; // 内核线程，不用拷贝参数
    PRINTK("new pid %d\n", t->pid); // 这里变成console_printk直接炸，因为console还没初始化
}

struct task_struct* kthread_create(int (*threadfn)(void *data), void *data, int prio, const char* name) {

    enum intr_status old_status = intr_disable();  /* TODO 实现锁之后删掉这个 */

    struct task_struct* new_thread = get_free_page(KERNEL_POOL);   // PCB占一个页框

    init_thread(new_thread, threadfn, data, prio, name);

    list_push_back(&ready_list, &new_thread->task_tag); 
    
    intr_set_status(old_status);

    return new_thread;
}

void thread_yield() {
    intr_disable();
    struct task_struct* current = get_current();
    current->state = READY;
    list_push_back(&ready_list, &current->task_tag);
    schedule();
    intr_enable(); // schedule完要开中断,因为不是由中断进入的(中断进入的话出中断会自动开中断)
}

void schedule() {
    /* 通过中断门描述符（而不是调用门任务门）进入中断后会自动屏蔽外部中断(IF位置0)，返回后置1 */
    ASSERT(intr_get_status() == INTR_OFF); 

    struct task_struct* current = get_current();

    // 时间片到期
    if (current->state == RUNNING) {
        current->ticks = current->prio;       
        current->state = READY;

        list_push_back(&ready_list, &current->task_tag);
    } else {
        if (current->state == EXIT) {
            PRINTK("pid %d exit code %d\n", current->pid, current->exit_code);
            free_pid(current->pid);
            free_page(current); // 退出态，回收最后的进程描述符
        }
    }

    struct task_struct* next;
    
    if (list_empty(&ready_list)) {       
        next = idle;
    } else {
        next = elem2entry(struct task_struct, task_tag, list_pop_front(&ready_list));
    }
    ASSERT(next!=NULL);
    next->state = RUNNING;
    process_activate(next);

    switch_to(current, next);
}

static void make_main_thread() {
    struct task_struct* main = 0xc009e000;
    
    main->stack_magic = STACK_MAGIC;
    strcpy(main->pname, "main");
    main->state = RUNNING;
    main->stack = 0xc009f000 - sizeof (struct intr_stack) - sizeof (struct thread_switch_data);
    main->prio = 10;
    main->pid = 0;
    main->mm = NULL;
}

void thread_init() {
    PRINTK("thread_init start\n");
    make_main_thread();
    pid_init();
    alloc_pid(); // main的
    list_init(&ready_list);
    idle = kthread_create(idle_thread, NULL, 10, "idle");
    ASSERT(idle != NULL);
    PRINTK("thread_init end\n");
}

uint32_t sys_getpid(void) {
   // printk("get pid:%d\n", get_current()->pid);
    return get_current()->pid;
}