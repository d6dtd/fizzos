#ifndef __DEVICE_CONSOLE_H
#define __DEVICE_CONSOLE_H

void console_init(void);
void console_printk(char* format, ...);
int console_print(char* buf, int count);
int console_print_by_color(char* buf, int count, char color);
#endif