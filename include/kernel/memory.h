#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "bitmap.h"
#include "fizzint.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"
#include "list.h"
#include "buddy.h"

#define KERNEL_VADDR_START  0xc0000000  // 物理地址加这个就是内核虚拟地址

struct paddr_pool {
    struct bitmap bmap;
    uint32_t paddr_start; 
    uint32_t byte_size;   /* if remove */
};

/* 
 * 用首部总内存只能2G，用byte_size可以4G
 */
struct vaddr_pool {
    struct bitmap bmap; 
    uint32_t vaddr_start;
    uint32_t vaddr_end;
    uint32_t byte_size; 
    uint32_t vaddr_head; /* 首部 */
};

extern void* pdt_addr;

void* kmalloc(size_t size);
void* kzalloc(size_t size);
void kfree(const void* addr);
void memory_init(void);

void* get_pde(void* vaddr);
void* get_pte(void* vaddr);
void* get_paddr(void* vaddr);
/* 
 * TODO 以后删掉，用mmap或brk
 */
void* get_user_page(void);
void* get_user_paddr_page(void);
void* get_kernel_paddr_page(void);
#endif