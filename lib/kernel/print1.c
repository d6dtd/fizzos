#include "print2.h"

#define MAX_ROW 25
#define MAX_COL 80

// TODO 同步 加锁
static uint16_t offset = 0; // 0 - 1999
static uint8_t color = F_BRIGHT_WHITE | B_BLACK;

void put_char3(char c)
{
    if (c == '\r' || c == '\n') {
        offset = (offset + MAX_COL - 1) / MAX_COL * MAX_COL;
    } else if (c == '\b') {
        --offset;
        put_char2(((uint32_t)32 << 16) | ((uint32_t)(color) << 24) | offset);
    } else {
        put_char2(((uint32_t)c << 16) | ((uint32_t)(color) << 24) | offset);
        offset++;    
    }
    set_cursor2(offset);
}

void set_color(uint8_t col)
{
    color = col;
}

uint8_t get_color()
{
   return color;
}

void set_foreground(uint8_t col)
{
    color = col | (color & 0xf0);
}

void set_background(uint8_t col)
{
    color = col | (color & 0x0f);
}

void put_str2(char* s)
{
    while (*s) {
        put_char3(*s);
        s++;
    }
}

void put_str_with_color(char* s, uint8_t co)
{
    uint8_t tmp = color;
    color = co;
    while (*s) {
        put_char3(*s);
        s++;
    }
    color = tmp;
}