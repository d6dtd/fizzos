#ifndef __KERNEL_BUDDY_H
#define __KERNEL_BUDDY_H

#define KERNEL_POOL 0
#define USER_POOL   1
#define MAX_ORDER 10

#define PAGE_ALIGN(addr) ((addr) / 4096 * 4096)

uint32_t buddy_init(uint32_t saddr, uint32_t total_mem_size);
struct page* alloc_pages(uint32_t flag, int order);
void __free_pages(struct page* pg, int order);
void* get_free_page(uint32_t flag);
void* get_free_pages(uint32_t flag, int order);
void* get_zeroed_page(uint32_t flag);
void free_page(void* addr);
void free_pages(void* addr, int order);
void* pg2paddr(struct page* pg);
struct page* paddr2pg(void* addr);
#endif