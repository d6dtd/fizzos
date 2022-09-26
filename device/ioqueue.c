#include "ioqueue.h"
#include "../include/kernel/memory.h"
#include "fizzstr.h"
#include "print.h"

void ioqueue_init(struct ioqueue* q, int buf_size) {
    q->buf = kmalloc(buf_size);
    q->buf_size = buf_size + 1; // 留个空白位分辨empty and full
    mutex_lock_init(&q->lock);
    cond_init(&q->char_cond);
    q->head = buf_size;
    q->tail = 0;
    memset(q->buf, 0, buf_size + 1);
}

void ioqueue_destroy(struct ioqueue* q) {
    kfree(q->buf);
}

char ioqueue_getchar(struct ioqueue* q) {

    mutex_acquire(&q->lock);
    
    ASSERT(q->head >= 0);
    while ((q->tail + q->buf_size - q->head - 1) % q->buf_size == 0) {
        cond_wait(&q->char_cond, &q->lock);
    }
    q->head = (q->head + 1) % q->buf_size;
    char c = *(q->buf + q->head);

    mutex_release(&q->lock);

    return c;
}

void ioqueue_putchar(struct ioqueue* q, char c) {
    mutex_acquire(&q->lock);
    ASSERT(q->tail >= 0);
    if ((q->tail + q->buf_size - q->head - 1) % q->buf_size < q->buf_size - 1) {
        *(q->buf + q->tail) = c;
        q->tail = (q->tail + 1) % q->buf_size;
    } else {
        // 满了直接扔掉，否则得添加一个条件变量
    }
    if (c == '\n') {
        cond_signal(&q->line_cond);
    }
    cond_signal(&q->char_cond);
    mutex_release(&q->lock);
}

// 实现putchar的条件变量后再
bool ioqueue_try_putchar(struct ioqueue* q, char c) {
    /*if (sema_trywait(&q->sem_empty)) {
        mutex_acquire(&q->write_lock);

        ASSERT(q->tail >= 0);
        *(q->buf + q->tail) = c;
        q->tail = (q->tail + 1) % q->buf_size;

        mutex_release(&q->write_lock);
        sema_post(&q->sem);

        return true;
    } else {
        return false;
    }*/
}

// 为退格键准备的
void ioqueue_pop_back_char(struct ioqueue* q) {
    mutex_acquire(&q->lock);

    if ((q->tail + q->buf_size - q->head - 1) % q->buf_size > 0) {
        q->tail = (q->tail + q->buf_size - 1) % q->buf_size;
    }

    mutex_release(&q->lock);
}

/*
 * 返回读到的字符数，包括换行符
 * 传入count == -1 代表读到换行符
 */ 
int ioqueue_getline(struct ioqueue* q, char* dest, int count)
{
    mutex_acquire(&q->lock);

    /* 
     * 扫一遍看看有无换行符，没有就cond_wait
     */
    bool flag = true;
    int head, tail, size;
    char* buf;
    while (flag) {
        head = q->head;
        tail = q->tail;
        size = q->buf_size;
        buf = q->buf; // 避免多次访存
        for (int i = (head + 1) % size; i != tail; i = (i + 1) % size) {
            if (buf[i] == '\n') {
                flag = false;
                break;
            }
        }
        if (flag == true) cond_wait(&q->line_cond, &q->lock);
    }
    int c = 0;
    head = (head + 1) % size;
    while ((count == -1 || c < count) && (*dest = *(buf + head)) != '\n') {
        ASSERT((tail + size - head - 1) % size > 0); // 至少剩下一个换行符
        dest++;
        head = (head + 1) % size;
        c++;
    }

    // 剩下的扔掉
    while (*(buf + head) != '\n') {
        ASSERT((tail + size - head - 1) % size > 0); // 至少剩下一个换行符
        head = (head + 1) % size;
    }
    q->head = head; // 此时head指向'\n'，q->head代表从q->head + 1开始读取

    mutex_release(&q->lock);
    return c + 1; // +1是换行符
}