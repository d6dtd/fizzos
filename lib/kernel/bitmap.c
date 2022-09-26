#include "bitmap.h"
#include "debug.h"
#include "print.h"
/* 
 * 初始化为0，多余的位置1，该函数不会分配内存
 */
void bitmap_init(struct bitmap* bmap, uint32_t size){
    ASSERT(size > 0); /* 不然下面会出错 */
    bmap->byte_size = DIV_ROUND_UP(size, 8);
    for(int i = 0; i < bmap->byte_size - 1; i++){
        *(bmap->bits + i) = 0;
    }
    uint8_t last_byte = 0;
    for(int i = 7; i > (size + 7) % 8; i--){
        last_byte |= 1<<(i);
    }
    *(bmap->bits + bmap->byte_size - 1) = last_byte;

    mutex_lock_init(&bmap->lock);
}


void bitmap_set(struct bitmap* bmap, uint32_t idx){
    mutex_acquire(&bmap->lock);

    /* 加上assert */
    ASSERT(!(*(bmap->bits + idx / 8) & 1<<(idx % 8)));
    *(bmap->bits + idx / 8) |= 1<<(idx % 8);

    mutex_release(&bmap->lock);
}

void bitmap_reset(struct bitmap* bmap, uint32_t idx){
    mutex_acquire(&bmap->lock);
    
    ASSERT(*(bmap->bits + idx / 8) & 1<<(idx % 8));
    *(bmap->bits + idx / 8) &= ~((uint8_t)1<<(idx % 8));
    
    mutex_release(&bmap->lock);
}

int32_t bitmap_scan(struct bitmap* bmap){
    mutex_acquire(&bmap->lock);
    
    uint32_t size = bmap->byte_size % 4;
    uint32_t* bits = (uint32_t*)bmap->bits;
    int32_t idx = 0;
    while(idx < bmap->byte_size / 4){
        if(*bits != -1){
            for(int i = 0; i < 32; i++){
                if(((*bits) & (1<<i)) == 0){
                    mutex_release(&bmap->lock);
                    return idx * 32 + i;
                }
            }
        }          
        bits++;
        idx++;
    }
    for(int i = 0; i < bmap->byte_size % 4 * 8; i++){
        if(((*bits) & (1<<i)) == 0){
            mutex_release(&bmap->lock);
            return idx * 32 + i;
        }
    }  
    
    mutex_release(&bmap->lock);
    PANIC; /* 到时再添加处理 */
    return -1;
}

int32_t bitmap_scan_series(struct bitmap* bmap, int32_t cnt) {
    mutex_acquire(&bmap->lock);

    ASSERT(cnt > 0);
    if(cnt == 1) {
        return bitmap_scan(bmap);
    }
    
    uint32_t* bits = (uint32_t*)bmap->bits;
    int32_t idx = 0;
    int32_t count = cnt;
    while(idx < bmap->byte_size / 4) {
        while(idx < bmap->byte_size / 4 && *bits != -1) {
            for(int i = 0; i < 32; i++) {
                if (((*bits) & (1<<i)) == 0) {
                    if(--count == 0) {
                        mutex_release(&bmap->lock);
                        return idx * 32 + i - cnt + 1;
                    } 
                } else {
                    count = cnt;
                }
            }
            bits++;
            idx++;
        }    
        if(idx >= bmap->byte_size / 4) break;
        bits++;
        idx++;
    }
    
    for(int i = 0; i < bmap->byte_size % 4 * 8; i++) {
        for(int i = 0; i < 32; i++) {
            if (((*bits) & (1<<i)) == 0) {
                if(--count == 0) {
                    mutex_release(&bmap->lock);
                    return idx * 32 + i - cnt + 1;
                }
            } else {
                count = cnt;
            }
        }
    }   
    PANIC; 
    mutex_release(&bmap->lock);
    return -1;
}

// TODO 可以优化
void bitmap_set_series(struct bitmap* bmap, int32_t start_idx, int32_t cnt) {
    mutex_acquire(&bmap->lock);

    ASSERT(cnt > 0);
    int n = 0;
    while(n < cnt) {
        bitmap_set(bmap, start_idx + n);
        n++;
    }

    mutex_release(&bmap->lock);
}

void bitmap_reset_series(struct bitmap* bmap, int32_t start_idx, int32_t cnt) {
    mutex_acquire(&bmap->lock);

    ASSERT(cnt > 0);
    int n = 0;
    while(n < cnt) {
        bitmap_reset(bmap, start_idx + n);
        n++;
    }
    
    mutex_release(&bmap->lock);
}

int bitmap_scan_set(struct bitmap* bmap) {
    mutex_acquire(&bmap->lock);

    int idx = bitmap_scan(bmap);
    ASSERT (idx >= 0);
    bitmap_set(bmap, idx);

    mutex_release(&bmap->lock);

    return idx;
}

int bitmap_scan_set_series(struct bitmap* bmap, int32_t cnt) {
    mutex_acquire(&bmap->lock);
    
    int idx = bitmap_scan_series(bmap, cnt);
    ASSERT (idx >= 0);
    bitmap_set_series(bmap, idx, cnt);

    mutex_release(&bmap->lock);

    return idx;
}

bool bitmap_is_set(struct bitmap* bmap, int32_t idx) {
    uint8_t* b = bmap->bits + idx / 8;

    return (*b & (1<<(idx % 8))) > 0; //转为true(1)或false(0)
}

// 返回取反后的结果
// TODO 增加效率
int bitmap_reverse(struct bitmap* bmap, int32_t idx)
{
    int ret;
    mutex_acquire(&bmap->lock);
    if (bitmap_is_set(bmap, idx)) {
        bitmap_reset(bmap, idx);
        ret = 0;
    } else {
        bitmap_set(bmap, idx);
        ret = 1;
    }
    mutex_release(&bmap->lock);
    return ret;
}