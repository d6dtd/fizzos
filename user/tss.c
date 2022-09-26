#include "tss.h"
#include "global.h"
#include "debug.h"
#include "print.h"

static struct tss_segment_32 tss;

void update_esp0(struct task_struct* thread) {
    tss.esp0 = ((uint32_t)thread & 0xfffff000) + PG_SIZE;
}

static uint64_t make_desc(uint32_t base, bool G, bool D_B, 
            uint32_t limit, int DPL, bool S, int TYPE) {
    ASSERT(DPL >= 0 && DPL <= 3);

    uint32_t low = 0, high = 0;
    low = (limit & 0xffff) | ((base & 0xffff) << 16);
    high = ((base & 0xff0000) >> 16) | ((TYPE & 0xf) << 8) | ((S == true) << 12) \
            | (DPL << 13) | (DESC_P << 15) | (limit & 0xf0000) | (DESC_AVL) << 20 | \
            (D_B << 22) | (G << 23) | (base & 0xff000000);
    
    return ((uint64_t)high << 32) + low;
}

/* 
 * 在GDT添加tss段描述符，加载tss段描述符到tr寄存器
 * 添加dpl为3的代码段和数据段
 */
void tss_init() {
    PRINTK("tss_init begin\n");

    int tss_size = sizeof(tss);
    tss.io_map = tss_size; // 代表没有IO位图 TODO 用到IO位图时在添加
    tss.ss0 = SELECTOR_K_STACK;

    uint64_t tss_desc = make_desc(&tss, 0, DESC_D_32, tss_size - 1, DESC_DPL_0, DESC_S_SYS, DESC_TYPE_TSS);
    *(uint64_t*)(0xc0000920) = tss_desc;

    uint64_t user_code = make_desc(0, DESC_G_4K, DESC_D_32, 0xfffff, DESC_DPL_3, DESC_S_CODE, DESC_TYPE_CODE);
    *(uint64_t*)(0xc0000928) = user_code;

    uint64_t user_data = make_desc(0, DESC_G_4K, DESC_D_32, 0xfffff, DESC_DPL_3, DESC_S_DATA, DESC_TYPE_DATA);
    *(uint64_t*)(0xc0000930) = user_data;

    uint64_t gdt_operand = (7 * 8 - 1) | ((uint64_t)(0xc0000900) << 16);

    asm volatile("lgdt %0": : "m"(gdt_operand));
    asm volatile("ltr %w0": : "r"(SELECTOR_TSS));

    PRINTK("tss_init end\n");
}