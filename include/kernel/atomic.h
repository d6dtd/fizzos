#ifndef __KERNEL_ATOMIC_H
#define __KERNEL_ATOMIC_H

// volatile 确保访存
#define atomic_read(v)	(*(volatile int *)&(v)->counter)
#define atomic_set(v, i) ((v)->counter = i)

typedef struct atomic {
    int counter;
} atomic_t;

/* 
 * 如果直接v->counter += i，并返回v->counter，
 * 返回时又要访存一次，此时可能该值被其他线程修改 
 * 所以用一个临时变量存储然后返回临时变量
 */
/* static inline int atomic_add_return(int i, atomic_t* v)
{
    int temp;

    enum intr_status old_status = intr_disable(void);
    temp = v->counter;
    temp += i;
    v->counter = temp;
    intr_set_status(old_status);
    
    return temp;    
}
// 未完成
static inline int atomic_sub_return(int i, atomic_t* v)
{
    int temp;

    enum intr_status old_status = intr_disable(void);
    temp = v->counter;
    temp -= i;
    v->counter = temp;
    intr_set_status(old_status);
    
    return temp;
} */

static inline void atomic_add(int i, atomic_t* v)
{
    asm volatile("addl %0, %1"::"r"(i),"m"(v->counter));
}

static inline void atomic_sub(int i, atomic_t* v)
{
    asm volatile("subl %0, %1"::"r"(i),"m"(v->counter));
}

// 大于0为1，小于等于0为0
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

static inline int atomic_inc_and_sete(atomic_t* v)
{
    unsigned char ret;
    asm volatile("incl %0; sete %1"
                 : "+m"(v->counter), "=m"(ret)
                 :
                 :"memory");
    return ret;
}

static inline int atomic_inc_and_setge(atomic_t* v)
{
    unsigned char ret;
    asm volatile("incl %0; setge %1"
                 : "+m"(v->counter), "=m"(ret)
                 :
                 :"memory");
    return ret;
}

// 大于等于0为1，小于0为0
static inline int atomic_dec_and_setge(atomic_t* v)
{
    unsigned char ret;
    asm volatile("decl %0; setge %1"
                 : "+m"(v->counter), "=m"(ret)
                 :
                 :"memory");
    return ret;
}

// 等于0为1，不等于0为0
static inline int atomic_dec_and_sete(atomic_t* v)
{
    unsigned char ret;
    asm volatile("decl %0; sete %1"
                 : "+m"(v->counter), "=m"(ret)
                 :
                 :"memory");
    return ret;
}

// 大于0为1，小于等于0为0
static inline int atomic_dec_and_setg(atomic_t* v)
{
    unsigned char ret;
    asm volatile("decl %0; setg %1"
                 : "+m"(v->counter), "=m"(ret)
                 :
                 :"memory");
    return ret;
}

static inline void atomic_inc(atomic_t* v)
{
    asm volatile("incl %0":"+m"(v->counter));
}

static inline void atomic_dec(atomic_t* v)
{
    asm volatile("decl %0":"+m"(v->counter));
}

static inline int atomic_xchg_return(atomic_t* v, int i)
{
    // 内联时i就是主调函数的变量了，所以加个memory
    asm volatile("xchg %0, %1":"+r"(i), "+m"(v->counter)::"memory"); 
    return i;
}

// 和mov差不多了
static inline void atomic_xchg(atomic_t* v, int i)
{
    asm volatile("xchg %0, %1":"+m"(v->counter):"r"(i)); 
}

#endif