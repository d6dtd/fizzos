#include "sync.h"
#include "interrupt.h"
#include "kthread.h"
#include "errno.h"
#include "print.h"
#include "../include/kernel/sched.h"

void slock_init(slock_t* sl)
{
    atomic_set(&sl->is_lock, 0);
}
void slock_lock(slock_t* sl)
{
    while (atomic_xchg_return(&sl->is_lock, 1)) { // 循环检查
        thread_yield();
    }
}
void slock_unlock(slock_t* sl)
{
    // 确保已上锁 TODO 可以用noreturn版本的，现在为了调试
    ASSERT(atomic_xchg_return(&sl->is_lock, 0));
}


void sema_init(sem_t* sem, int val)
{
    atomic_set(&sem->count, val);
    list_init(&sem->wait_list);
}

void sema_post(sem_t* sem)
{   
    // 加一之后还是负数或零，说明有线程阻塞了
    if (!atomic_inc_and_setg(&sem->count)) {
        enum intr_status s = intr_disable();
        thread_unblock(elem2entry(struct task_struct, task_tag, list_pop_front(&sem->wait_list)));
        intr_set_status(s);
    }
    
}

void sema_wait(sem_t* sem)
{
    enum intr_status s = intr_disable();

    if (!atomic_dec_and_setge(&sem->count)) {
        list_push_back(&sem->wait_list, &get_current()->task_tag);
        thread_block();
    }
    intr_set_status(s);
    
}

bool sema_trywait(sem_t* sem)
{
    bool success = true;
    enum intr_status s = intr_disable();
    if (!atomic_dec_and_setge(&sem->count)) {
        atomic_inc(&sem->count);
        success = false;
    }
    intr_set_status(s);
    return success;
}

void sema_destory(sem_t* sem)
{
    ASSERT(list_empty(&sem->wait_list));
}

void mutex_lock_init(mutex_lock_t* mutex)
{
    list_init(&mutex->wait_list);
    mutex->is_lock = false;
    mutex->holder = NULL;
    mutex->repeat = 0;
}

void mutex_acquire(mutex_lock_t* mutex)
{
    struct task_struct* current = get_current();
    /*
     * 锁释放时会置空，所以不会出现上一个线程释放后再申请并条件成立的情况
     * 其他线程判断时，要么空要么是持有锁的线程，都为false
     */
    if (mutex->holder == current) {
        mutex->repeat++;
    } else {
        bool flag = true;
        while (flag) {
            // 只能这样了，判断必须和插链表一起
            enum intr_status old_status = intr_disable();
            // 感觉是两个原子操作 xchg lock(m), flag(r); flag(r)->flag(m) 不过无所谓
            asm volatile("xchg %0, %1" // 加号表示先读出再写入
                         : "+r"(flag), "+m"(mutex->is_lock)
                         :
                         : "memory");
            if (flag) {
                list_push_back(&mutex->wait_list, &current->task_tag);
                thread_block();
            }
            intr_set_status(old_status);
        }

        mutex->holder = current;
        mutex->repeat = 1;
    }
}

void mutex_release(mutex_lock_t* mutex)
{
    ASSERT(mutex->holder == get_current());
    if (--mutex->repeat == 0) {
        mutex->holder = NULL; // 先置空再释放

        bool flag = false;
        enum intr_status old_status = intr_disable();
        asm volatile("xchg %0, %1"
                     : "+r"(flag), "+m"(mutex->is_lock)
                     : 
                     : "memory");
        ASSERT(flag == true); // 其实可以mov，但是为了DEBUG
        struct list_elem* elem = list_pop_front(&mutex->wait_list);
        intr_set_status(old_status);

        if (elem != NULL) {
            thread_unblock(elem2entry(struct task_struct, task_tag, elem));
        }
    } else if (mutex->repeat < 0) {
        PANIC;
    }
}

void mutex_lock_destory(mutex_lock_t* mutex)
{
    ASSERT(list_empty(&mutex->wait_list));
}

void cond_init(cond_t* cond)
{
    list_init(&cond->wait_list);
}

/* 
 * 为了解锁和等待原子操作，不然可能本线程解锁->其他线程获取锁->其他线程signal->本线程wait
 * 本来wait函数时在signal之前的，现在signal没唤醒线程，而wait还在等待
 */
void cond_wait(cond_t* cond, mutex_lock_t* mutex)
{
    enum intr_status old_status = intr_disable();

    ASSERT(!list_find(&cond->wait_list, &get_current()->task_tag));
    // 阻塞前释放锁，唤醒后加锁
    mutex_release(mutex);

    list_push_back(&cond->wait_list, &get_current()->task_tag);
    thread_block();
    intr_set_status(old_status);

    /*
     * 这一步不用原子操作，broadcast的时候全部唤醒，所以要加个锁，只让一个线程消费
     * cond_wait是等待条件成立，还没开始消费，消费之前加个锁
     */
    mutex_acquire(mutex);
}

/* 
 * 主调函数要保证先获取了锁(wait时传进来的)再调用该函数，
 */
void cond_signal(cond_t* cond)
{
    enum intr_status old_status = intr_disable();

    if (!list_empty(&cond->wait_list)) {
        thread_unblock(elem2entry(struct task_struct, task_tag, list_pop_front(&cond->wait_list)));
    }

    intr_set_status(old_status);
}

void cond_broadcast(cond_t* cond)
{
    enum intr_status old_status = intr_disable();

    while (!list_empty(&cond->wait_list)) {
        thread_unblock(elem2entry(struct task_struct, task_tag, list_pop_front(&cond->wait_list)));
    }

    intr_set_status(old_status);
}

void rwlock_init(rwlock_t* rwlock)
{
    list_init(&rwlock->read_waiter_list);
    list_init(&rwlock->write_waiter_list);
    rwlock->count = 0;
    rwlock->ready_write = false;
}

void rwlock_read_acquire(rwlock_t* rwlock)
{
    enum intr_status old_status = intr_disable();

    /*  
     * 正在写，或者有写操作在等待时，阻塞
     */
    while (rwlock->count < 0 || rwlock->ready_write == true) {
        list_push_back(&rwlock->read_waiter_list, &get_current()->task_tag);
        thread_block();
    }
    rwlock->count++;

    intr_set_status(old_status);
}

void rwlock_read_release(rwlock_t* rwlock)
{
    enum intr_status old_status = intr_disable();
    ASSERT(rwlock->count > 0);

    if (--rwlock->count == 0) {
        /* 写优先，如果有写操作，会把ready_write置为true */
        if (!list_empty(&rwlock->write_waiter_list)) {
            thread_unblock(elem2entry(struct task_struct, task_tag, list_pop_front(&rwlock->write_waiter_list)));
        } else if (!list_empty(&rwlock->read_waiter_list)) {
            thread_unblock(elem2entry(struct task_struct, task_tag, list_pop_front(&rwlock->read_waiter_list)));
        }
    }

    intr_set_status(old_status);
}

void rwlock_write_acquire(rwlock_t* rwlock)
{
    enum intr_status old_status = intr_disable();

    // 确保无操作正在进行
    while (rwlock->count != 0) {
        rwlock->ready_write = true;
        list_push_back(&rwlock->write_waiter_list, &get_current()->task_tag);
        thread_block();
    }
    rwlock->count--;

    intr_set_status(old_status);
}

void rwlock_write_release(rwlock_t* rwlock)
{
    enum intr_status old_status = intr_disable();

    ASSERT(rwlock->count == -1 && rwlock->ready_write == true);
    rwlock->count++;

    /* 写优先，先唤醒写的，此时ready_write为true，不用改变 */
    if (!list_empty(&rwlock->write_waiter_list)) {
        thread_unblock(elem2entry(struct task_struct, task_tag, list_pop_front(&rwlock->write_waiter_list)));
    } else {
        rwlock->ready_write = false;
        while (!list_empty(&rwlock->read_waiter_list)) {
            thread_unblock(elem2entry(struct task_struct, task_tag, list_pop_front(&rwlock->read_waiter_list)));
        }
    }

    intr_set_status(old_status);
}