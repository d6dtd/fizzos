#ifndef __KERNEL_MM_STRUCT_H
#define __KERNEL_MM_STRUCT_H
#include "list.h"
#include "./atomic.h"

/*
 * vm_flags in vm_area_struct, see mm_types.h.
 */
#define VM_READ		0x00000001	/* currently active flags */
#define VM_WRITE	0x00000002
#define VM_EXEC		0x00000004
#define VM_SHARED	0x00000008

/* mprotect() hardcodes VM_MAYREAD >> 4 == VM_READ, and so for r/w/x bits. */
#define VM_MAYREAD	0x00000010	/* limits for mprotect() etc */
#define VM_MAYWRITE	0x00000020
#define VM_MAYEXEC	0x00000040
#define VM_MAYSHARE	0x00000080

struct page {
    uint32_t    flags;
    atomic_t    count;
    atomic_t    mapcount;
    uint32_t    private;
    void*       v; // 先占位
    // struct address_space *mapping;
    uint32_t    index;
    struct list_elem    lru;
    void*       vaddr;
};

struct vm_area_struct {
    struct mm_struct*   vm_mm;
    uint32_t    vm_start;
    uint32_t    vm_end;
    struct vm_area_struct* vm_next;
    uint32_t    vm_page_prot;  // 访问控制权限
    uint32_t    vm_flags;
    uint32_t    vm_pgoff;       // 文件中的偏移量，4K对齐
    struct file*    vm_file;    // 被映射的文件(如果存在)
    void*       vm_private_data;
};

struct mm_struct {
    struct vm_area_struct *mmap;    // 虚拟内存链表
    struct vm_area_struct *mmap_cache;  //   下次访问同一个的可能性很大，局部性
    int users;                      // 共享内存的用户数量
    void* pg_dir;               // 页目录表的虚拟地址
    int mm_count;
    int map_count;
    uint32_t start_code;
    uint32_t end_code;
    uint32_t start_data;
    uint32_t end_data;
    uint32_t start_brk;
    uint32_t brk;
    uint32_t start_stack;   // main的返回地址之上
    uint32_t arg_start;
    uint32_t arg_end;
    uint32_t env_start;
    uint32_t env_end;
    uint32_t flags;
    uint32_t total_vm;      // 全部页面数目
};

#endif