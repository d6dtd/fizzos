#ifndef __TSS_H
#define __TSS_H
#include "fizzint.h"
#include "../include/kernel/sched.h"

struct tss_segment_32 {
	uint32_t prev_task_link;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt_selector;
	uint16_t t;
	uint16_t io_map;
};

void update_esp0(struct task_struct* thread);
void tss_init(void);
#endif