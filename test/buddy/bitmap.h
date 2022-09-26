typedef unsigned uint32_t;
typedef unsigned long long uint64_t;
typedef int int32_t;
typedef unsigned char uint8_t;
#define ASSERT(a)
#define NULL 0
#define PANIC 
#define bool int
#define true 1
#define false 0

struct bitmap {
    uint8_t* bits;
    uint32_t byte_size;   /* 字节的长度 */
};
#define OFFSET(type, attr) ((uint64_t)(&((type*)0)->attr))
#define elem2entry(type, attr, elem) ((type*)((uint64_t)elem - OFFSET(type, attr)))
#define NULL ((void*)0)
#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))
#define bool int
#define true 1
#define false 0
/* 
 * 初始化为0，多余的位置1，该函数不会分配内存
 */
void bitmap_init(struct bitmap* bmap, uint32_t size){
    bmap->byte_size = DIV_ROUND_UP(size, 8);
    for(int i = 0; i < bmap->byte_size - 1; i++){
        *(bmap->bits + i) = 0;
    }
    uint8_t last_byte = 0;
    for(int i = 7; i > (size + 7) % 8; i--){
        last_byte |= 1<<(i);
    }
    *(bmap->bits + bmap->byte_size - 1) = last_byte;

}


void bitmap_set(struct bitmap* bmap, uint32_t idx){

    /* 加上assert */
    *(bmap->bits + idx / 8) |= 1<<(idx % 8);

}

void bitmap_reset(struct bitmap* bmap, uint32_t idx){
    
    *(bmap->bits + idx / 8) &= ~((uint8_t)1<<(idx % 8));
    
}

int32_t bitmap_scan(struct bitmap* bmap){
    
    uint32_t size = bmap->byte_size % 4;
    uint32_t* bits = (uint32_t*)bmap->bits;
    int32_t idx = 0;
    while(idx < bmap->byte_size / 4){
        if(*bits != -1){
            for(int i = 0; i < 32; i++){
                if(((*bits) & (1<<i)) == 0){
                    return idx * 32 + i;
                }
            }
        }          
        bits++;
        idx++;
    }
    for(int i = 0; i < bmap->byte_size % 4 * 8; i++){
        if(((*bits) & (1<<i)) == 0){
            return idx * 32 + i;
        }
    }  
    
    return -1;
}

int32_t bitmap_scan_series(struct bitmap* bmap, int32_t cnt) {

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
                    return idx * 32 + i - cnt + 1;
                }
            } else {
                count = cnt;
            }
        }
    }   
    return -1;
}

// TODO 可以优化
void bitmap_set_series(struct bitmap* bmap, int32_t start_idx, int32_t cnt) {

    int n = 0;
    while(n < cnt) {
        bitmap_set(bmap, start_idx + n);
        n++;
    }

}

void bitmap_reset_series(struct bitmap* bmap, int32_t start_idx, int32_t cnt) {

    int n = 0;
    while(n < cnt) {
        bitmap_reset(bmap, start_idx + n);
        n++;
    }
    
}

int bitmap_scan_set(struct bitmap* bmap) {

    int idx = bitmap_scan(bmap);
    bitmap_set(bmap, idx);


    return idx;
}

int bitmap_scan_set_series(struct bitmap* bmap, int32_t cnt) {
    
    int idx = bitmap_scan_series(bmap, cnt);
    bitmap_set_series(bmap, idx, cnt);


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
    if (bitmap_is_set(bmap, idx)) {
        bitmap_reset(bmap, idx);
        ret = 0;
    } else {
        bitmap_set(bmap, idx);
        ret = 1;
    }
    return ret;
}