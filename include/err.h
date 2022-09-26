#ifndef __ERR_H
#define __ERR_H
#include "errno.h"
#include "fizzint.h"

static inline int PTR_ERR(void* ptr)
{
    return (int)ptr;
}

// 如果一个指针有错误，它将返回一个-ERRNO
static inline int IS_ERR(void* ptr)
{
    return (uint32_t)ptr >= (uint32_t)(-MAX_ERRNO); // -MAX_ERRNO <= ptr <= 0
}

static inline void* ERR_PTR(int err)
{
    return (void*)err;
}

#endif