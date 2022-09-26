#ifndef __DEVICE_TIME_H
#define __DEVICE_TIME_H
#include "fizzint.h"
#include "../include/kernel/sched.h"
void timer_init(void);
void mtime_sleep(uint32_t m_seconds);
void sleep(uint32_t seconds);
#endif