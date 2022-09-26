#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H

#ifndef NODEBUG
#define ASSERT(cond) if(!(cond)) panic(__FILE__, __LINE__,__func__)
#else 
#define ASSERT(cond)
#endif
/* TODO */

#define PANIC  panic(__FILE__, __LINE__,__func__)
void panic(const char*, int, const char*);
#endif