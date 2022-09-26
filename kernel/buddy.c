#include "../include/kernel/memory.h"
#include "print.h"
#include "fizzstr.h"
#include "global.h"
#include "../include/kernel/sched.h"
#include "console.h"
#include "mm_types.h"
#include "buddy.h"
#include "interrupt.h"

struct free_area {
    int nr_free;
    struct bitmap bmap;
    struct list pg_list;
};

static struct page* mem_map;
static int pg_cnt;
static int user_start_idx; // 用户内存开始的索引

struct free_area kernel_area[MAX_ORDER + 1];
struct free_area user_area[MAX_ORDER + 1];

struct page* alloc_pages(uint32_t flag, int order)
{
    enum intr_status old_status = intr_disable();
    struct free_area* area;
    if (flag & USER_POOL) {     // 如果修改USER_POOL的宏，注意这个与符号
        area = user_area;
    } else {
        area = kernel_area;
    }
    int o = order;
    
    ASSERT(o >= 0 && o <= MAX_ORDER);
    while (o <= MAX_ORDER) {
        if (area[o].nr_free > 0) {
            area[o].nr_free--;
            struct page* pg = elem2entry(struct page, lru, list_pop_front(&area[o].pg_list));
            int idx = pg - mem_map;
            int size = (1 << o) / 2;
            pg->private = order;
            bitmap_reverse(&area[o].bmap, idx >> (o + 1));
            while (--o >= order) {
                list_push_back(&area[o].pg_list, &(idx + size + mem_map)->lru);
                area[o].nr_free++;
                bitmap_reverse(&area[o].bmap, idx>>(o + 1));
                size /= 2;
            }
            //printk("order:%d alloc start idx:%d\n", order, pg - mem_map);
            atomic_set(&pg->count, 1);
            intr_set_status(old_status);
            return pg;
        }
        o++;
    }
    PANIC;
    intr_set_status(old_status);
    return NULL;
}

// TODO flag是否可以不用
void __free_pages(struct page* pg, int order)
{
    // 如果大于0直接返回 什么时候加一呢？
    if (atomic_dec_and_setg(&pg->count)) {
        return;
    }
    enum intr_status old_status = intr_disable();
    //printk("free:idx:%d order:%d\n", pg - mem_map, pg->private);
    struct free_area* area;
    if (pg - mem_map >= user_start_idx) {     // 如果修改USER_POOL的宏，注意这个与符号
        area = user_area;
    } else {
        ASSERT(pg - mem_map < user_start_idx);
        area = kernel_area;
    }

    ASSERT(order == pg->private);
    int idx = pg - mem_map;
    int tmp = order;
    int ret;
    while(order < MAX_ORDER && (ret = bitmap_reverse(&area[order].bmap, idx >> (order + 1)) == 0)) {
        
        list_delete(&area[order].pg_list, &(mem_map + (idx ^ (1 << order)))->lru);
        area[order].nr_free--;
        idx &= ~(1 << order);
        order++;
    }
    list_push_front(&area[order].pg_list, &(mem_map + idx)->lru); // 放在前面，下次优先用
    area[order].nr_free++;
    intr_set_status(old_status);
}

void* get_free_page(uint32_t flag)
{
    struct page* pg = alloc_pages(flag, 0);
    return (pg - mem_map) * PG_SIZE + KERNEL_VADDR_START;
}

void* get_free_pages(uint32_t flag, int order)
{
    struct page* pg = alloc_pages(flag, order);
    return (pg - mem_map) * PG_SIZE + KERNEL_VADDR_START;
}

void* get_zeroed_page(uint32_t flag)
{
    struct page* pg = alloc_pages(flag, 0);
    void* addr = (pg - mem_map) * PG_SIZE + KERNEL_VADDR_START;
    memset(addr, 0, PG_SIZE);
    return addr;
}

void free_page(void* addr)
{
    ASSERT(((uint32_t)addr & 0x00000fff) == 0);
    __free_pages(paddr2pg(get_paddr(addr)), 0);
}

void free_pages(void* addr, int order)
{
    ASSERT(((uint32_t)addr & 0x00000fff) == 0);
    __free_pages(paddr2pg(get_paddr(addr)), order);
}

void* pg2paddr(struct page* pg)
{
    return (pg - mem_map) * PG_SIZE;
}
struct page* paddr2pg(void* paddr)
{
    return ((uint32_t)paddr / PG_SIZE + mem_map);
}

/* 
 * 输入起始地址及总内存，输出初始化后的地址
 */
uint32_t buddy_init(uint32_t saddr, uint32_t total_mem_size)
{
    mem_map = saddr + KERNEL_VADDR_START; // 页描述符数组的位置
    pg_cnt = total_mem_size / PG_SIZE; 
    saddr += pg_cnt * sizeof (struct page);

    int nr_max_order = pg_cnt >> MAX_ORDER;     // 最大order的数量
    ASSERT(nr_max_order >= 0);
    int kernel_nr = nr_max_order / 4;           // 内核四分之一
    int user_nr = nr_max_order - kernel_nr;     // 用户用剩下的
    user_start_idx = kernel_nr * (1 << MAX_ORDER);
    PRINTK("nr_total:%d kernel_nr:%d user_nr:%d\n", nr_max_order, kernel_nr, user_nr);
    // 初始化内核内存池
    kernel_area[MAX_ORDER].nr_free = kernel_nr;
    for (int i = 0; i < kernel_nr; i++) {
        list_push_back(&kernel_area[MAX_ORDER].pg_list, &((mem_map + (1 << MAX_ORDER) * i)->lru));
    }

    
    int nr = kernel_nr;
    for (int i = MAX_ORDER; i >= 0; i--) {
        int bmap_len = DIV_ROUND_UP(nr, 8);
        kernel_area[i].bmap.bits = saddr + KERNEL_VADDR_START;
        bitmap_init(&kernel_area[i].bmap, nr);
        saddr += bmap_len;
        nr *= 2;
    }

    // 初始化用户内存池
    user_area[MAX_ORDER].nr_free = user_nr;
    for (int i = kernel_nr; i < nr_max_order; i++) {
        list_push_back(&user_area[MAX_ORDER].pg_list, &((mem_map + (1 << MAX_ORDER) * i)->lru));
    }

    nr = user_nr; 
    for (int i = MAX_ORDER; i >= 0; i--) {
        int bmap_len = DIV_ROUND_UP(nr, 8);
        user_area[i].bmap.bits = saddr + KERNEL_VADDR_START;
        bitmap_init(&user_area[i].bmap, nr);
        saddr += bmap_len;
        nr *= 2;
    }

    return saddr;
}