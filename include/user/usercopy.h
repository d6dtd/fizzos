#ifndef __USER_USERCOPY_H
#define __USER_USERCOPY_H
#include "fizzint.h"
#include "global.h"

// 失败返回-1
int32_t copy_to_user(void __user *to, const void *from, unsigned long n);
int32_t copy_from_user(void  *to, const void __user *from, unsigned long n);
int32_t strncpy_from_user(void  *to, const void __user *from, unsigned long n);
#endif