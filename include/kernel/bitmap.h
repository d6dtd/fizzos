#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H
#include "fizzint.h"
#include "global.h"
#include "sync.h"

struct bitmap {
    uint8_t* bits;
    uint32_t byte_size;   /* 字节的长度 */
    mutex_lock_t lock;  // 用位图肯定得用锁
};

void bitmap_init(struct bitmap* bmap, uint32_t size);
void bitmap_set(struct bitmap* bmap, uint32_t idx);
void bitmap_reset(struct bitmap* bmap, uint32_t idx);
int32_t bitmap_scan(struct bitmap* bmap);
int32_t bitmap_scan_series(struct bitmap* bmap, int32_t cnt);
void bitmap_set_series(struct bitmap* bmap, int32_t start_idx, int32_t cnt);
void bitmap_reset_series(struct bitmap* bmap, int32_t start_idx, int32_t cnt);
int bitmap_scan_set(struct bitmap* bmap);
int bitmap_scan_set_series(struct bitmap* bmap, int32_t cnt);
bool bitmap_is_set(struct bitmap* bmap, int32_t idx);
int bitmap_reverse(struct bitmap* bmap, int32_t idx);
#endif