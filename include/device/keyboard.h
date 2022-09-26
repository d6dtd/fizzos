#ifndef __DEVICE_KEYBOARD_H
#define __DEVICE_KEYBOARD_H

void keyboard_init(void);
char keyboard_read(void);
int keyboard_readline(char* dest, int count);
#endif