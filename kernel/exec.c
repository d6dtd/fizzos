#include "exec.h"
#include "../include/fs/fs.h"
#include "../include/kernel/elf.h"
#include "debug.h"
#include "fizzstr.h"
#include "print.h"
#include "../include/kernel/sched.h"
#include "mm_types.h"
#include "../include/fcntl.h"
#include "../include/err.h"
#include "../include/errno.h"
#include "vma.h"
#include "buddy.h"
#include "file.h"
#include "process.h"

static char elf_head[201];
static char elf_magic[16] = {0x7f, 0x45, 0x4c, 0x46, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static void start_thread(uint32_t new_eip, uint32_t new_esp)
{
    PRINTK("new_eip:%x new_esp:%x\n", new_eip, new_esp);
    struct intr_stack* proc_stack = (uint32_t)get_current() + PG_SIZE - sizeof(struct intr_stack);
    proc_stack->edi = proc_stack->esi =\
    proc_stack->ebp = proc_stack->esp_dummy =\
    proc_stack->ebx = proc_stack->edx =\
    proc_stack->ecx = proc_stack->eax = 0;

    proc_stack->cs_old = SELECTOR_U_CODE;
    proc_stack->gs = 0;
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
    proc_stack->eip_old = new_eip;
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->esp_old = new_esp;
    proc_stack->ss_old = SELECTOR_U_STACK;
    PRINTK("start_thread:%x\n", start_thread);
}
/*
void print_elf()
{
    Elf32_Ehdr* head = elf_head;
    ASSERT(memcmp(head->e_ident, elf_magic, 16) == 0);
     printk("Entry point address:                 0x%x\n", head->e_entry);
    printk("Start of program headers:            %d (bytes into file)\n", head->e_phoff);
    printk("Start of section headers:            %d (bytes into file)\n", head->e_shoff);
    printk("Size of this header:                 %d (bytes)\n", head->e_ehsize);
    printk("Size of program headers:             %d (bytes)\n", head->e_phentsize);
    printk("Number of program headers:           %d\n", head->e_phnum);
    printk("Size of section headers:             %d (bytes)\n", head->e_shentsize);
    printk("Number of section headers:           %d\n", head->e_shnum);
    printk("Section header string table index:   %d\n", head->e_shstrndx);

    Elf32_phdr* phead = (Elf32_phdr*)(elf_head + head->e_phoff);
    printk("Type  Offset  VirtAddr  PhysAddr  FileSiz  MemSiz  Flg Align\n"); // TODO 实现打印对齐
    for (int i = 0; i < head->e_phnum; i++) {
        printk("%d     %x    %x    %x   %x     %x     %d   %x\n", \
        (phead + i)->p_type, (phead + i)->p_offset, (phead + i)->p_vaddr, \
        (phead + i)->p_paddr, (phead + i)->p_filesz, (phead + i)->p_memsz, \
        (phead + i)->p_flags, (phead + i)->p_align);
    } 
     loff_t pos = (phead + 1)->p_offset;
    file_read(current->files->fd_array[fd], buf2, PG_SIZE, &pos);
    for (int i = 0; i < 40; i++) {
        printk("%d ", *(buf + (head->e_entry - 0x8048000) + i));
        printk("%d ", *(buf2 + head->e_entry - 0x8049000 + i));
        ASSERT(*(buf + (head->e_entry - 0x8048000) + i) == *(buf2 + head->e_entry - 0x8049000 + i));
        if ((i + 1) % 10 == 0) {
            put_char('\n');
        }
    } 
}*/

/* 
 * 本来应该是由伙伴算法分配页框，预先分配一些单个页框给缓存，然后缓存分配单个页框
 * FIXME:释放页框 -1即可 先实现fork
 */
static void exec_mmap(struct mm_struct* mm)
{
    if(!mm) return;
    // 清除线性区的页框
    struct vm_area_struct *cur = mm->mmap, *next;
    mm->mmap = NULL;
    uint32_t tmp = 0;
    /*
    for (struct vm_area_struct* vm = mm->mmap; vm != NULL; vm = vm->vm_next) {
        ASSERT(vm->vm_start >= USER_VADDR_START && vm->vm_end <= TASK_SIZE);
        printk("start:%x end:%x\n", vm->vm_start, vm->vm_end);
    }*/
    while (cur) {
        //printk("cur vm_start:%x\n", cur->vm_start);
        uint32_t addr = PAGE_ALIGN(cur->vm_start);
        if (addr >= tmp) {
            if ((*(uint32_t*)get_pde(addr) & 1 == 1) && \
            (*(uint32_t*)get_pte(addr) & 1 == 1)) {
                //printk("free page addr:%x\n", addr);
                free_page(addr);    // 都是一个个分配的，要一个个释放,如果已映射则释放
            }
            addr += PG_SIZE;
        }
        while (addr < cur->vm_end) {
            if ((*(uint32_t*)get_pde(addr) & 1) == 1 && \
            (*(uint32_t*)get_pte(addr) & 1) == 1) {
                //printk("free page addr:%x\n", addr);
                free_page(addr);    // 如果已映射则释放
            }
            addr += PG_SIZE;
        }
        tmp = addr;
        next = cur->vm_next;
        kfree(cur);
        cur = next;    
    }
    // 清除页表
    void* pg_dir = mm->pg_dir;
    for (int i = 0; i < 768; i++) {
        // 如果存在，就把页表释放
        if ((*((uint32_t*)pg_dir + i) & 1) == 1) {
            //printk("free page addr:%x\n", (*((uint32_t*)pg_dir + i) & 0xfffff000) + KERNEL_VADDR_START);
            free_page((*((uint32_t*)pg_dir + i) & 0xfffff000) + KERNEL_VADDR_START); // 获取页表物理地址并转化为虚拟地址
            *((uint32_t*)pg_dir + i) = 0;
        }
    }
    // FIXME:页目录表和内存块描述符留着，不然当前程序都无法执行
}

static void flush_old_files(struct files_struct* f)
{
    // 关闭后相应的位清0
    uint32_t close_on_exec = f->close_on_exec;
    for (int i = 0; i < 32; i++) {
        if ((close_on_exec & (1 << i)) == 1) {
            file_close(f->fd_array[i]);
            close_on_exec &= ~(1 << i);
            f->open_fds &= ~(1 << i);
        }
    }
    f->close_on_exec = close_on_exec;
}

static void flush_old_exec(struct task_struct* current)
{
    // 信号处理
    // 线程组
    // 如果与其他进程共享，调用unshare_files()拷贝一份files_struct结构
    exec_mmap(current->mm); // 释放分配给进程的内存描述符、所有线性区及所有页框，并清除进程的页表
    // 将可执行文件路径名赋给进程描述符的comm字段
    // 信号相关
    flush_old_files(current->files);  // 关闭所有打开的文件，close_on_exec
}

// TODO: pathname需要检查一下
int do_exec(char* pathname, const char* const* argv, const char* const* envp)
{
    PRINTK("do exec path:%s %d\n", pathname, intr_get_status());
    struct task_struct* current = get_current();
    struct file* file = filp_openat(AT_FDCWD, pathname, O_RDWR);
    if (IS_ERR(file)) {
        PRINTK("file not found\n");
        return PTR_ERR(file);
    }
    // 检查文件是否可执行

    // 将文件路径名、命令行参数及环境串拷贝到一个或多个新分配的页框中，以后分给用户 暂时是一个页框
    void* arg = get_zeroed_page(USER_POOL) + PG_SIZE;
    void* stack = TASK_SIZE;
    char** s = kzalloc(sizeof (char*) * 10); // 暂时支持十个命令行参数
    int len = 0;
    int argc = 0;
    while (argv && *argv) { // TODO 要先拷贝进内核
        PRINTK("argv:%x\n", argv);
        PRINTK("*argv:%x\n", *argv);
        if (argc >= 10) PANIC;
        len = strlen(*argv);
        arg -= (len + 1 + 3) / 4 * 4; // 4字节对齐 留一个'\0'
        stack -= (len + 1 + 3) / 4 * 4;
        strcpy(arg, *argv);
        s[argc] = stack; // 用户的虚拟地址才行
        argc++;
        argv++;
    }
    arg -= 4 * 11; // 十个命令行参数指针和一个argc
    stack -= 4 * 11;
    *(uint32_t*)arg = argc;
    PRINTK("argc:%d\n", argc);
    memcpy(arg + 4, s, sizeof (char*) * 10);

    loff_t pos = 0;
    file_read(file, elf_head, 200, &pos);
    Elf32_Ehdr* head = (Elf32_Ehdr*)elf_head;
    if (memcmp(head->e_ident, elf_magic, 16) != 0) {
        PANIC;
        return -ENOEXEC;
    }
    Elf32_phdr* phead = (Elf32_phdr*)(elf_head + head->e_phoff);
    // 读段
    // 动态链接程序相关
    flush_old_exec(current); // 万一关闭之后，执行本函数的时候又产生缺页就寄
    if (current->files->map_file) {
        file_close(current->files->map_file);
        current->files->map_file = file;
    }
    /* 
     * 为进程的用户态堆栈分配一个新的线性区描述符并插入地址空间，
     * 把命令行参数和环境变量串缩在的页框分配给新的线性区（在页表中填上还是用其他方式？）
     */
    //setup_arg_pages(); 
    // 此时mm里的段也被清除干净
    if (!current->mm) { // 帮内核线程创建一个
        PRINTK("kernel thread create mm\n");
        current->mm = kzalloc(sizeof (struct mm_struct));
        void* pg_dir = get_zeroed_page(KERNEL_POOL);
        memcpy(pg_dir + 768 * 4, pdt_addr + 768 * 4, PG_SIZE / 4);
        *((uint32_t*)pg_dir + 1023) = (uint32_t)get_paddr(pg_dir) | PG_SUPERVISOR | PG_P | PG_RWE;
        current->mm->pg_dir = pg_dir;
        asm volatile("mov %0, %%cr3"::"r"(get_paddr(pg_dir)):"memory");
    }
    struct mm_struct* mm = current->mm;
    for (int i = 0; i < head->e_phnum; i++) {
        if ((phead + i)->p_type == ELF_LOAD) {
            if ((phead + i)->p_flags == ELF_READ) { // 只读
                do_mmap(file, (phead + i)->p_vaddr, (phead + i)->p_memsz, \
                0, VM_READ, (phead + i)->p_offset);    // 暂时不处理BSS段 
            } else if ((phead + i)->p_flags == (ELF_READ | ELF_WRITE)) {
                mm->start_data = (phead + i)->p_vaddr;
                mm->end_data = (phead + i)->p_vaddr + (phead + i)->p_memsz;
                do_mmap(file, (phead + i)->p_vaddr, (phead + i)->p_memsz, \
                0, VM_READ | VM_WRITE, (phead + i)->p_offset);    // TODO:暂时不处理BSS段 
            } else if ((phead + i)->p_flags == (ELF_READ | ELF_EXEC)) {
                mm->start_code = (phead + i)->p_vaddr;
                mm->end_code = (phead + i)->p_vaddr + (phead + i)->p_memsz;
                do_mmap(file, (phead + i)->p_vaddr, (phead + i)->p_memsz, \
                0, VM_READ | VM_EXEC, (phead + i)->p_offset);    // 暂时不处理BSS段 
            }
        }
    }
    // 创建栈段 8K
    do_mmap(NULL, TASK_SIZE - PG_SIZE * 2, PG_SIZE * 2, 0, VM_READ | VM_WRITE, 0);
    
    // 先给栈段分配4k，放命令行参数
    uint32_t* pde = get_pde(TASK_SIZE-PG_SIZE);
    if ((*pde & 1) == 0) {
        PRINTK("alloc pde\n");
        *pde = (uint32_t)get_paddr(get_zeroed_page(KERNEL_POOL)) | PG_P | PG_RWE | PG_USER;
    }
    uint32_t* pte = get_pte(TASK_SIZE-PG_SIZE);
    // 页表的页框从内核中分配，其他页框从用户池分配
    *pte = (uint32_t)get_paddr(arg) | PG_P | PG_RWE | PG_USER;

    // 动态链接程序
    // TODO do_brk 给bss段分配
    /* for (struct vm_area_struct* vm = mm->mmap; vm != NULL; vm = vm->vm_next) {
        printk("start:%x end:%x pid:%d\n", vm->vm_start, vm->vm_end, current->pid);
        ASSERT(vm->vm_start >= USER_VADDR_START && vm->vm_end <= TASK_SIZE && vm->vm_start < vm->vm_end);
    } */
    start_thread(head->e_entry, stack);
    
    return 0; 
}

int sys_execve(const char* pathname, const char* const* argv, const char* const* envp)
{
    return do_exec(pathname, argv, envp);
}