#ifndef __LIB_KERNEL_PRINT2_H
#define __LIB_KERNEL_PRINT2_H
#include "fizzint.h"
#include "fizzarg.h"

#define B_BLACK             0
#define B_BLUE              0x10
#define B_GREEN             0x20
#define B_CYAN              0x30
#define B_RED               0x40
#define B_MAGENTA           0x50
#define B_BROWN             0x60
#define B_WHITE             0x70
#define B_SHINE_BLACK       0x80
#define B_SHINE_BLUE        0x90
#define B_SHINE_GREEN       0xa0
#define B_SHINE_CYAN        0xb0
#define B_SHINE_RED         0xc0
#define B_SHINE_MAGENTA     0xd0
#define B_SHINE_BROWN       0xe0
#define B_SHINE_WHITE       0xf0
#define F_BLACK             0x00
#define F_BLUE              0x01
#define F_GREEN             0x02
#define F_CYAN              0x03
#define F_RED               0x04
#define F_MAGENTA           0x05
#define F_BROWN             0x06
#define F_WHITE             0x07
#define F_GRAY              0x08
#define F_BRIGHT_BLUE       0x09
#define F_BRIGHT_GREEN      0x0a
#define F_BRIGHT_CYAN       0x0b
#define F_BRIGHT_RED        0x0c
#define F_BRIGHT_PURPLE     0x0d
#define F_YELLOW            0x0e
#define F_BRIGHT_WHITE      0x0f

struct char_asci {
    uint16_t addr;  // 相对于0xb8000的偏移
    uint8_t ch;
    uint8_t color;
}__attribute__((packed));

void put_char2(uint32_t c);
void put_char3(char c);
void set_cursor2(uint16_t offset);
void set_start_addr(uint16_t offset);
void put_str2(char* s);
void put_str_with_color(char* s, uint8_t co);
void set_foreground(uint8_t col);
void set_background(uint8_t col);
void set_color(uint8_t col);
uint8_t get_color();
#endif