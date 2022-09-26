#include "timer.h"
#include "io.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"


#define HZ	   100
#define CLOCK_TICK_RATE   1193180
#define LATCH ((CLOCK_TICK_RATE + HZ / 2) / HZ)
#define COUNTER0_VALUE	   CLOCK_TICK_RATE / HZ
#define CONTRER0_PORT	   0x40
#define COUNTER0_NO	   0
#define COUNTER_MODE	   2
#define READ_WRITE_LATCH   3
#define PIT_CONTROL_PORT   0x43

#define mil_seconds_per_intr (1000 / HZ)

uint32_t ticks = 0;          // ticks是内核自中断开启以来总共的嘀嗒数

// /* 把操作的计数器counter_no、读写锁属性rwl、计数器模式counter_mode写入模式控制寄存器并赋予初始值counter_value */
// static void frequency_set(uint8_t counter_port, \
// 			  uint8_t counter_no, \
// 			  uint8_t rwl, \
// 			  uint8_t counter_mode, \
// 			  uint16_t counter_value) {
// /* 往控制字寄存器端口0x43中写入控制字 */
//    outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
// /* 先写入counter_value的低8位 */
//    outb(counter_port, (uint8_t)counter_value);
// /* 再写入counter_value的高8位 */
//    outb(counter_port, (uint8_t)(counter_value >> 8));
// }

void init_PIT(void) {
   outb(0x34, 0x43);
   outb(LATCH & 0xff, 0x40);
   outb(LATCH >> 8, 0x40);
}

/* 时钟的中断处理函数 */
static void intr_timer_handler(void) {

   struct task_struct* current = get_current();

   if (current->stack_magic != STACK_MAGIC) {
       PRINTK("name : %s\n", current->pname);
   }

   ASSERT(current->stack_magic == STACK_MAGIC); // 检查栈是否溢出

   current->elapsed_ticks++; // 记录此线程占用的cpu时间嘀
   ticks++; //从内核第一次处理时间中断后开始至今的滴哒数,内核态和用户态总共的嘀哒数
   if (current->ticks == 0) {	  // 若进程时间片用完就开始调度新的进程上cpu
      schedule(); 
   } else {				  // 将当前进程的时间片-1
      current->ticks--;
   }
}


// 以tick为单位的sleep,任何时间形式的sleep会转换此ticks形式
static void ticks_to_sleep(uint32_t sleep_ticks) {
   uint32_t start_tick = ticks;

   while (ticks - start_tick < sleep_ticks) {	   // 若间隔的ticks数不够便让出cpu
      enum intr_status old_status = intr_disable(); 
   //   printk("start %d ticks %d\n", start_tick, ticks);
      thread_yield();
      intr_set_status(old_status);
   }
}

// 以毫秒为单位的sleep   1秒= 1000毫秒
void mtime_sleep(uint32_t m_seconds) {
  uint32_t sleep_ticks = DIV_ROUND_UP(m_seconds, mil_seconds_per_intr);
  ASSERT(sleep_ticks > 0);
  ticks_to_sleep(sleep_ticks); 
}

void sleep(uint32_t seconds) {
   mtime_sleep(seconds * 1000);
}

/* 初始化PIT8253 */
void timer_init() {
   put_str("timer_init start\n");
   init_PIT();
   /* 设置8253的定时周期,也就是发中断的周期 */
  // frequency_set(CONTRER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
   register_handler(0x20, intr_timer_handler);
   put_str("timer_init done\n");
}
