#include "mm_types.h"
#include "../include/errno.h"
#include "../include/kernel/sched.h"
#include "vma.h"
#include "print.h"
#include "debug.h"
#include "fizzstr.h"

// 返回第一个vm_end大于addr的结构体
struct vm_area_struct* find_vma(struct mm_struct* mm, uint32_t addr)
{
    ASSERT(mm);
    struct vm_area_struct* vma = mm->mmap;
    while (vma) {
        if (vma->vm_end > addr) {
            return vma;
        }
        vma = vma->vm_next;
    }
    return NULL;
}

// 返回第一个与区间相交的结构体  [start, end) [vm_start, vm_end)  end = vm_start时也不相交
struct vm_area_struct* find_inter_section(struct mm_struct* mm, \
                        uint32_t start_addr, uint32_t end_addr)
{
    struct vm_area_struct* vma = find_vma(mm, start_addr);
    if (vma && end_addr <= vma->vm_start)
        vma = NULL;
    return vma;
}

/* 
 * file offset: 文件及偏移量
 * addr: 从何处开始查找一个空闲的区间
 * len : 线性地址区间的长度
 * prot: 页的访问权限(可能没用)
 * flag: 标志
 * TODO:检测文件属性与参数，如果属性不一致需要页对齐 offset必须4K对齐
 */
int do_mmap(struct file *file, uint32_t addr, uint32_t len, \
                    uint32_t prot, uint32_t flag, uint32_t offset)
{
    if (len == 0) {
        PANIC;
        return 0;
    }
    if (addr == 0) {
        PANIC;
        // start addr = 0代表让系统自动分配
    }
    if (addr + len > TASK_SIZE) {
        PANIC;
    }
    int ret = -1;
	struct mm_struct* mm = get_current()->mm;
    struct vm_area_struct* vma = mm->mmap;
    struct vm_area_struct* v = kzalloc(sizeof (struct vm_area_struct));
    if (vma == NULL) {
        mm->mmap = v;
        v->vm_next = NULL;
    } else { // 从addr开始找一个空闲区
        while (vma->vm_next && (vma->vm_next->vm_start < addr || \
            vma->vm_next->vm_start - MAX(vma->vm_end, addr) < len)) {
            addr = MAX(addr, vma->vm_end);
            vma = vma->vm_next;
        }
        if (vma->vm_end + len >= 0xc0000000) {
            PANIC;
            goto out;
        }
        // FIXME: 感觉很大几率可以合并，用到再实现
        if (vma->vm_end == addr && vma->vm_next && vma->vm_next->vm_start == addr + len) {
            PANIC;
        } else if (vma->vm_end == addr) {
            PANIC;
        } else if (vma->vm_next && vma->vm_next->vm_start == addr + len) {
            PANIC;
        } else {
            v->vm_next = vma->vm_next;
            vma->vm_next = v;
        }
    }
    v->vm_file = file; // file为空时，匿名映射
    v->vm_pgoff = offset;
    v->vm_start = addr;
    v->vm_end = addr + len;
    v->vm_flags = VM_READ | VM_WRITE | VM_EXEC;
    v->vm_page_prot = prot;
    //printk("start:%x end:%x pid:%d\n", v->vm_start, v->vm_end, get_current()->pid);
    ASSERT(v->vm_start >= 0x8048000 && v->vm_end <= TASK_SIZE && v->vm_start < v->vm_end);
    return 0;
out:
    kfree(v);
    return ret;
}

int dup_mmap(struct mm_struct* old, struct mm_struct* new)
{
    struct vm_area_struct* old_vma = NULL;
    struct vm_area_struct* new_vma = NULL;
    if (old->mmap) {
        new->mmap = kmalloc(sizeof (struct vm_area_struct));
        memcpy( new->mmap, old->mmap, sizeof (struct vm_area_struct));
        old_vma = old->mmap;
        new_vma = new->mmap;
    }

    while (old_vma) {
        if (old_vma->vm_next) {
            new_vma->vm_next = kmalloc(sizeof (struct vm_area_struct));
            memcpy(new_vma->vm_next, old_vma->vm_next, sizeof (struct vm_area_struct));
        }
        old_vma = old_vma->vm_next;
        new_vma = new_vma->vm_next;
    }
    return 0;
}