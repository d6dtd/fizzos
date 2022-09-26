#include<stdio.h>
#include <unistd.h>
#define atomic_set(v, i) ((v)->counter = i)
#define atomic_read(v)	(*(volatile int *)&(v)->counter)
typedef struct atomic {
    int counter;
} atomic_t;
typedef struct semaphore {
    atomic_t count;
} sem_t;
void sema_init(sem_t* sem, int val);
void sema_post(sem_t* sem);
void sema_wait(sem_t* sem);

static inline int atomic_inc_and_test(atomic_t *v)
{
    unsigned char c;

    asm volatile("incl %0; sete %1"
                 : "+m"(v->counter), "=qm"(c)
                 :
                 : "memory");
    return c != 0;
}

static inline int atomic_inc_and_setg(atomic_t* v)
{
    /* 
     * +m 说明为可读可写，inc内存肯定是可读可写的
     * =m set类指令不需要读，只写
     * 由于set类只有r8/m8，所以返回值必须为uchar类型，如果是32位，只赋值8位，其他位可能有脏数据
     */
    unsigned char ret;
    asm volatile("incl %0; setg %1"
                 : "+m"(v->counter), "=m"(ret)
                 :
                 :"memory");
    return ret;
}
static inline int atomic_dec_and_setge(atomic_t* v)
{
    int ret = 0;
    asm volatile("decl %0; setge %1"
                 : "+m"(v->counter), "=m"(ret)::"memory");
    return ret;
}
static inline int atomic_xchg_return(atomic_t* v, int i)
{
    asm volatile("xchg %0, %1":"+r"(i), "+m"(v->counter)::"memory");
    return i;
}
static inline void atomic_xchg(atomic_t* v, int i)
{
    asm volatile("xchg %0, %1":"+m"(v->counter):"r"(i)); 
}
sem_t sem;
int main(){
	int a = 100, b = 456;
    asm volatile("xchg %0, %1":"+r"(a), "+m"(b)::"memory");
    printf("%d %d\n", a, b);
}

void sema_init(sem_t* sem, int val)
{
    atomic_set(&sem->count, val);
}

void sema_post(sem_t* sem)
{
    printf("sema_count:%d\n", atomic_read(&sem->count));
    // 加一之后还是负数或零，说明有线程阻塞了
    if (!atomic_inc_and_setg(&sem->count)) {
        printf("sema_wake!!\n");
    } else {
        printf("sema_post!!\n");
    }
}

void sema_wait(sem_t* sem)
{
    printf("sema_count:%d\n", atomic_read(&sem->count));
    if (!atomic_dec_and_setge(&sem->count)) {
        printf("sema_wait!!\n");
    } else {
        printf("sema_down!!\n");
    }
}