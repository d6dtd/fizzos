#ifndef __LIB_KERNEL_PRINT_H
#define __LIB_KERNEL_PRINT_H
#include "fizzint.h"
#include "fizzarg.h"

void put_char(uint8_t char_asci);
void put_str(const char* message);
void put_int(uint32_t num);	 // 以16进制打印
int printk(char* format, ...);
int vprintk(char* format, va_list ap);
int sprintk(char* dest, char* format, ...);
int snprintk(char* dest, size_t size, char* format, ...);
void set_cursor(uint32_t cursor_pos);
void cls_screen(void);

#endif