#include "usercopy.h"
#include "fizzstr.h"
#include "print.h"
// TODO 检验内存
int32_t copy_to_user(void __user *to, const void *from, unsigned long n)
{   
    PRINTK("copy %d bytes to user\n", n);
    memcpy(to, from, n);
    return n;
}
int32_t copy_from_user(void  *to, const void __user *from, unsigned long n)
{
    PRINTK("copy %d bytes from user\n", n);
    memcpy(to, from, n);
    return n;
}

// 先计算n的最大长度，然后再调用该函数
int32_t strncpy_from_user(void  *to, const void __user *from, unsigned long n)
{
    strncpy(to, from, n);
    return n;
}