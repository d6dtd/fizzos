#include "../include/kernel/memory.h"
#include "print.h"
#include "fizzstr.h"
#include "global.h"
#include "../include/kernel/sched.h"
#include "console.h"
#include "mm_types.h"
#include "buddy.h"

#define MAX_BLOCK_SIZE 2032

// 直接物理地址加上K_VADDR_START等于内核虚拟地址
#define K_VADDR_START 0xc0000000

#define PDE_IDX(addr) (addr>>22)
#define PTE_IDX(addr) (addr>>12 & 0x3ff)


/* 
 * 初始化7个内存块链表，粒度分别为32 64 116 238 508 1016 2032字节
 * 链表连接着每个页的内存块描述符，内存块描述符记录着已分配数量，为0时回
 * 收该页，删去节点。
 * 
 * 每个页用前32字节作为内存块描述符
 */
struct mem_block_desc {
    struct list_elem page_elem; // 页之间的连接
    struct list block_list;     // 页内 内存块的连接
    union {
        int count;                  // large为false时代表内存块数量
        int order;                  // large为true时代表页框order
    };
    int remain;
    int mlist_idx;
    bool large;   // 为true时代表页框
};

struct list mblock_part[7];
struct list mblock_full[7];

// 对齐4字节
int mem_block_size[7] = {32, 64, 116, 238, 508, 1016, 2032};
int mem_block_count[7] = {127, 63, 35, 17, 8, 4, 2};

void* pdt_addr;

static void pool_init(uint32_t total_mem_size){
    PRINTK("total mem_size:%x\n", total_mem_size);
    pdt_addr = 0x100000 + KERNEL_VADDR_START;
    uint32_t saddr = 0x200000; // 起始地址,1M内核及1目录表255页表
    
    ASSERT(total_mem_size > 512 * PG_SIZE);
    ASSERT(total_mem_size <= 1024 * 1024 * 1024); // 小于1G，不然不能直接映射
    // 前2M内存的页表已分配，0xc00以上的目录项也全部填满
    for (int i = 512; i < 1024; i++) {
        *(uint32_t*)(0x101000 + i * 4) = i * PG_SIZE | PG_P | PG_SUPERVISOR; // 内核的页表不用设置权限
    }

    uint32_t mem_per_ptt = 1024 * PG_SIZE; // 每个页表分配的内存
    for (int i = 1; i < DIV_ROUND_UP(total_mem_size, mem_per_ptt); i++) { // 向上取整
        uint32_t ptt = 0x101000 + PG_SIZE * i;
        for (int j = 0; j < 1024; j++) {
            *(uint32_t*)(ptt + j * 4) = (i * 1024 * PG_SIZE + j * PG_SIZE) | PG_P | PG_SUPERVISOR; 
        }
    }

    // 不存在的物理地址的页表项全部清0
    for (uint32_t i = 0x101000 + total_mem_size / PG_SIZE * 4; i < 0x200000; i+=4) {
        *(uint32_t*)i = 0;
    }

    saddr = buddy_init(saddr, total_mem_size);
    PRINTK("saddr:%x\n", saddr);
    int alloc_pg = (saddr + PG_SIZE - 1) / PG_SIZE;  // 向上取整，已分配的页框

    PRINTK("alloc_pg:%d\n", alloc_pg);
    PRINTK("count:%d\n", DIV_ROUND_UP(alloc_pg, 1 << MAX_ORDER));
    for (int i = 0; i < DIV_ROUND_UP(alloc_pg, 1 << MAX_ORDER); i++) {
        alloc_pages(KERNEL_POOL, 10);   // 因为要保证是前面连续的地址
    }
     

    for (int i = 0; i < 7; i++) {
        list_init(&mblock_part[i]);
        list_init(&mblock_full[i]);
    }
}


/* 
 * 获取页目录项的指针
 * 暂时实现内核版本的
 * TODO
 */
void* get_pde(void* vaddr) {
    return (0xfffff<<12) + (PDE_IDX((uint32_t)vaddr)<<2);
}
/* 
 * 获取页表项的指针
 * 暂时实现内核版本的
 * TODO
 */
void* get_pte(void* vaddr) {
    return  (0x3ff<<22) + (PDE_IDX((uint32_t)vaddr)<<12) + (PTE_IDX((uint32_t)vaddr)<<2);
}

/* 
 * 获取虚拟地址对应的物理地址
 */
void* get_paddr(void* vaddr) {
    return (*(uint32_t*)get_pte(vaddr) & 0xfffff000) | ((uint32_t)vaddr & 0x00000fff);
}
/*
void map(void* vaddr_start, void* paddr_start) {
    uint32_t* pde_addr = (uint32_t*)get_pde(vaddr_start);

        if((*pde_addr & 1) == 0) {
            // 如果是用户进程，创建页目录项和页表
            *pde_addr = (uint32_t)get_paddr(get_free_page()) | PG_P | PG_RWE | PG_USER;
        }

        uint32_t* pte_addr = (uint32_t*)get_pte(vaddr_start);
        ASSERT((*pte_addr & 1) == 0); //TODO 正常来说内存一开始应该全0的，但是不知道用了虚拟机会怎样
        *pte_addr = (uint32_t)paddr_start | PG_P | PG_RWE | PG_USER;

    }   
}*/

// void unmap(void* vaddr) {
//     uint32_t* pde_addr = (uint32_t*)get_pde(vaddr);
//     if(*pde_addr & 1 == 0) {
//         PANIC; /* 内核的页目录项存在位肯定为1 */
//     }
//     uint32_t* pte_addr = (uint32_t*)get_pte(vaddr);
//     ASSERT((*pte_addr & 1) == 1);
//     *pte_addr &= 0xfffffffe;

//     // 如果是进程可能还需要回收页表
// }


/* 
 * 分配的内存在物理上也是连续的，
 * 最小32B，(最大128KB 未限制)
 */
void* kmalloc(size_t size) {

    enum intr_status old_status = intr_disable();
     
    struct mem_block_desc* desc;
    
    if (size > MAX_BLOCK_SIZE) {
        int cnt = DIV_ROUND_UP(size + 32, PG_SIZE);
        int order = 0;
        while (cnt > 1) {
            cnt /= 2;
            order++;
        }
        desc = get_free_pages(KERNEL_POOL, order);
        desc->order = order;
        desc->large = true;

        intr_set_status(old_status);

        return (uint32_t)desc + 32;
    }

    int idx;
    for (idx = 0; idx < 7; idx++) {
        if (size <= mem_block_size[idx]) {
            break;
        }
    }
    ASSERT(idx < 7);
    
    // TODO 可以改为堆或者改成单向链表
    if (list_empty(&mblock_part[idx])) {
        /* 
         * 获取一页，创建描述符，获取内存块
         */
        desc = get_free_page(KERNEL_POOL);
        memset(desc, 0, sizeof desc);
        list_push_back(&mblock_part[idx], &desc->page_elem);
        desc->count = mem_block_count[idx];
        desc->remain = desc->count - 1;  // 现在分配一个
        desc->mlist_idx = idx;
        desc->large = false;

        list_init(&desc->block_list);
        for (int i = 0; i < desc->count; i++) {
            // 0-31是描述符结构体占的位置，32-4096是内存块，多余的去掉
            list_push_back(&desc->block_list, (struct list_elem*)((uint32_t)desc + 32 + mem_block_size[idx] * i));
        }
        list_push_back(&mblock_part[idx], &desc->page_elem);    /* 放进空闲链表中 */

    } else {
        /* 
         * 获取描述符，获取内存块，remain--，返回
         */
        struct list_elem* page_elem = list_front(&mblock_part[idx]);
        desc = elem2entry(struct mem_block_desc, page_elem, page_elem);

        if (--(desc->remain) == 0) {
            list_delete(&mblock_part[idx], page_elem);
            list_push_back(&mblock_full[idx], page_elem);
        }
    }
    intr_set_status(old_status);

    return (void*)list_pop_front(&desc->block_list);
}

void* kzalloc(size_t size)
{
    void* addr = kmalloc(size);
    memset(addr, 0, size);
    return addr;
}

/* 
 * 分配的地址在虚拟地址上连续，物理地址上不一定
 */
void* vmalloc(size_t size) {
    return NULL;
}


void kfree(const void* addr) {

    struct mem_block_desc* desc = (uint32_t)addr & 0xfffff000;

    if (desc->large == true) {
        free_pages(desc, desc->order);
        return;
    } 

    int idx = desc->mlist_idx;
    /* 
     * 如果可以在full中找到该描述符，删掉该描述符，添加到part链表中，剩余数加一。
     * 如果可以在part中找到该描述符，剩余数加一，如果等于count，从part中删掉该描述符，回收页面，
     * 否则把addr添加到block_list上
     */
    if (list_find(&mblock_full[idx], &desc->page_elem)) {
        ASSERT(list_find(&mblock_part[idx], &desc->page_elem) == false);

        list_delete(&mblock_full[idx], &desc->page_elem);
        list_push_back(&mblock_part[idx], &desc->page_elem);

        ASSERT(desc->remain == 0);
        desc->remain++;

        list_push_back(&desc->block_list, (struct list_elem*)addr);

    } else {

        ASSERT(list_find(&mblock_part[idx], &desc->page_elem));
        ASSERT(desc->remain < desc->count);

        ++desc->remain;
        if (desc->remain == desc->count) {
            list_delete(&mblock_part[idx], &desc->page_elem);
            free_page(desc);
        } else {
            list_push_back(&desc->block_list, (struct list_elem*)addr);
        }
    }
}

/* 
 * linux把物理页用结构体表示，如果要像linux那样实现
 * 要改很多代码，所以先暂时不搞
 * 只要实现缺页或异常把进程终止就行了
 */
void page_not_present(uint32_t addr)
{
    struct task_struct* current = get_current();
    PRINTK("page not present addr:%x pid:%d\n", addr, current->pid);
    if (addr >= 0xc0000000 || addr < 0x08048000) {
        printk("Segment fault\n");
        while(1);
    }
    struct mm_struct* mm = current->mm;
    struct vm_area_struct* vas = mm->mmap;
    while (vas) {
        if (vas->vm_end > addr) {
            //printk("match start_addr:%x end_addr:%x\n", vas->vm_start, vas->vm_end);
            if (vas->vm_file != NULL) {
                uint32_t* pde = get_pde(addr);
                if ((*pde & 1) == 0) {
                    //printk("install pde:%x     %x\n", get_paddr(pde), pde);
                    *pde = (uint32_t)get_paddr(get_zeroed_page(KERNEL_POOL)) | PG_USER | PG_P | PG_RWE;
                }
                void* buf = get_zeroed_page(USER_POOL);
                loff_t off = vas->vm_pgoff + (addr - vas->vm_start) / PG_SIZE * PG_SIZE; // 对齐并向下取整
                //printk("offset:%x\n", off);
                file_read(vas->vm_file, buf, PG_SIZE, &off);
                uint32_t* pte = get_pte(addr);
                ASSERT((*pte & 1) == 0);
                //printk("*pte:%x\n", *pte);
                *pte = (uint32_t)get_paddr(buf) | PG_USER | PG_P | PG_RWE;
            } else {
                // FIXME:暂时是栈
                uint32_t* pde = get_pde(addr);
                if ((*pde & 1) == 0) {
                    //printk("install pde:%x     %x\n", get_paddr(pde), pde);
                    *pde = (uint32_t)get_paddr(get_zeroed_page(KERNEL_POOL)) | PG_USER | PG_P | PG_RWE;
                }
                void* buf = get_zeroed_page(USER_POOL);
                uint32_t* pte = get_pte(addr);
                ASSERT((*pte & 1) == 0);
                *pte = (uint32_t)get_paddr(buf) | PG_USER | PG_P | PG_RWE;
                //printk("pte: %x *pte:%x\n", pte, *pte);
            }
            break;
        }
        vas = vas->vm_next;
    }
    ASSERT(vas != NULL);
}


void page_protect_fault(uint32_t addr)
{
    PRINTK("page protect fault addr:%x pid:%d %d\n", addr, get_current()->pid, intr_get_status());
    struct mm_struct* mm = get_current()->mm;
    struct vm_area_struct* vma =  find_vma(mm, addr);
    ASSERT(vma->vm_start <= addr); // 以后再处理
    if (vma->vm_flags & VM_WRITE) {
        ASSERT(*(uint32_t*)get_pde(addr) & PG_RWE); // 页目录项肯定可写 应该全部设为可写了
        uint32_t* pte = get_pte(addr);
        int tmp;
        if ((tmp = atomic_read(&paddr2pg(*pte)->count)) > 1) {
            //printk("count %d > 1\n", tmp);
            void* old_pg = (*pte & 0xfffff000) + K_VADDR_START;
            void* new_pg = get_free_page(USER_POOL); // 页框
            memcpy(new_pg, old_pg, PG_SIZE);
            free_page(old_pg);
            *pte = (uint32_t)get_paddr(new_pg) | PG_RWE | PG_P | PG_USER;
            ASSERT(tmp == atomic_read(&paddr2pg(*pte)->count) + 1);
        } else {
            //printk("count == 1\n");
            *pte |= PG_RWE | PG_USER; // 最后一次，不用拷贝了
        }
        //printk("pte:%x\n", pte);
       // while(1);
    } else {
        PANIC;
    }
    // TODO 终止进程
}

void memory_init(){
    put_str("memory_init\n");
    uint32_t total_mem_size = *(uint32_t*)(0xb00);
    pool_init(total_mem_size);
    put_str("memory_init_done\n");
}