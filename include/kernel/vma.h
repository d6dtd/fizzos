#ifndef __KERNEL_VMA_H
#define __KERNEL_VMA_H
#include "file.h"

int do_mmap(struct file *file, uint32_t addr, uint32_t len, \
                    uint32_t prot, uint32_t flag, uint32_t offset);
int dup_mmap(struct mm_struct* old, struct mm_struct* new);
struct vm_area_struct* find_vma(struct mm_struct* mm, uint32_t addr);
#endif