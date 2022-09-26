#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "sync.h"

/* TODO试试看有没有其他实现 */
// (tail + buf_size - head - 1) % buf_size 等于可用数量
struct ioqueue {
    mutex_lock_t lock;
    cond_t char_cond; // 输入字符时唤醒
    cond_t line_cond; // 输入换行时唤醒
    char* buf;
    int buf_size;
    int head;  // 初始化为buf_size - 1, 从(head + 1) % buf_size开始取
    int tail; // 下一个字符的位置
};

void ioqueue_init(struct ioqueue* q, int buf_size);
char ioqueue_getchar(struct ioqueue* q);
int ioqueue_getline(struct ioqueue* q, char* dest, int count);
void ioqueue_putchar(struct ioqueue* q, char c);
bool ioqueue_try_putchar(struct ioqueue* q, char c);
void ioqueue_pop_back_char(struct ioqueue* q);
#endif