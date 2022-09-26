#include "sync.h"
#include "../include/lib/kernel/print2.h"
#include "fizzarg.h"

static mutex_lock_t console_lock;
extern char console_buf[500];

void console_init() {
    mutex_lock_init(&console_lock);
}

void console_acquire() {
    mutex_acquire(&console_lock);
}

void console_release() {
    mutex_release(&console_lock);
}

void console_printk(char* format, ...) {
    console_acquire();
    va_list ap;
    va_start(ap, format);
    vprintk(format, ap);
    console_release();
}

int console_print(char* buf, int count)
{
    console_acquire();
    int c = snprintk(console_buf, count, buf);
    put_str(console_buf);
    console_release();
    return c;
}

//int console_print_by_color(char* buf, int count, char color)
//{
//    console_acquire();
//    int c = snprintk(console_buf, count, buf);
//
//    put_str_with_color(console_buf, color);
//    console_release();
//    return c;
//}