#ifndef __KERNEL_SYNC_H
#define __KERNEL_SYNC_H
#include "list.h"
#include "atomic.h"

/*
 * 和自旋锁不同的是，不会死等，而是调用schedule函数
 * 如果一个进程占用时间过长，其他进程就会一直schedule
 * 所以在用时短的地方使用，比如链表的插入和删除等
 * 释放锁之后schedule到的第一个进程将可以获得锁
 */
typedef struct slock {
    atomic_t is_lock;
} slock_t;

void slock_init(slock_t* sl);
void slock_lock(slock_t* sl);
void slock_unlock(slock_t* sl);

/* TODO destory and error handle */

typedef struct semaphore {
    atomic_t count;
    struct list wait_list;
} sem_t;

typedef struct mutex_lock {
    bool is_lock;
    struct list wait_list;
    struct task_struct* holder; // 只会在获取锁之后修改，改了也不影响其他线程的判断结果
    uint32_t repeat; // 加锁++，释放锁--，等于0时真正释放，同一时间只有一个线程在使用该变量
} mutex_lock_t;

/* TODO 搜怎么实现条件变量 */
typedef struct cond {
    struct list wait_list;
} cond_t;

/* TODO 搜怎么实现读写锁 */
typedef struct rwlock {
    struct list read_waiter_list;
    struct list write_waiter_list;
    int count;
    bool ready_write;
} rwlock_t;

void sema_init(sem_t* sem, int val);
void sema_post(sem_t* sem);
void sema_wait(sem_t* sem);
bool sema_trywait(sem_t* sem);

void mutex_lock_init(mutex_lock_t* mutex);
void mutex_acquire(mutex_lock_t* mutex);
void mutex_release(mutex_lock_t* mutex);

void cond_init(cond_t* cond);
void cond_wait(cond_t* cond, mutex_lock_t* mutex);
void cond_signal(cond_t* cond);
void cond_broadcast(cond_t* cond);

void rwlock_init(rwlock_t* rwlock);
void rwlock_read_acquire(rwlock_t* rwlock);
void rwlock_read_release(rwlock_t* rwlock);
void rwlock_write_acquire(rwlock_t* rwlock);
void rwlock_write_release(rwlock_t* rwlock);
#endif