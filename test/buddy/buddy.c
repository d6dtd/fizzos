#include "./list.h"
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#define MAX_ORDER 10

struct page {
    uint32_t    flags;
    int    count;
    int    mapcount;
    uint32_t    private;
    void*       v; // 先占位
    // struct address_space *mapping;
    uint32_t    index;
    struct list_elem    lru;
    void*       vaddr;
};

struct free_area {
    int nr_free;
    struct bitmap bmap;
    struct list pg_list;
};

struct page mem_map[0x400000];
int pg_cnt;

struct free_area farea[MAX_ORDER + 1];

struct page* buddy_alloc(int order)
{
    int o = order;
    
   // ASSERT(o >= 0 && o <= MAX_ORDER);
    while (o <= MAX_ORDER) {
        if (farea[o].nr_free > 0) {
            farea[o].nr_free--;
            struct page* pg = elem2entry(struct page, lru, list_pop_front(&farea[o].pg_list));
            int idx = pg - mem_map;
            int size = (1 << o) / 2;
            pg->private = order;
            bitmap_reverse(&farea[o].bmap, idx >> (o + 1));
            while (--o >= order) {
                list_push_back(&farea[o].pg_list, &(idx + size + mem_map)->lru);
                farea[o].nr_free++;
                //idx += size;
                bitmap_reverse(&farea[o].bmap, idx>>(o + 1));
                size /= 2;
            }
            printf("order:%d alloc start idx:%d\n", order, pg - mem_map);
            return pg;
        }
        o++;
    }
    //PANIC;
    assert(0);
    return NULL;
}

void buddy_free(struct page* pg)
{
    printf("free:idx:%d order:%d\n", pg - mem_map, pg->private);
    int order = pg->private;
    int idx = pg - mem_map;
    int tmp = order;
    int ret;
    while(order < MAX_ORDER && (ret = bitmap_reverse(&farea[order].bmap, idx >> (order + 1)) == 0)) {
        
        list_delete(&farea[order].pg_list, &(mem_map + (idx ^ (1 << order)))->lru);
        farea[order].nr_free--;
        idx &= ~(1 << order);
        order++;
    }
    list_push_front(&farea[order].pg_list, &(mem_map + idx)->lru); // 放在前面，下次优先用
    farea[order].nr_free++;
    
    /*
    // 按顺序排列
    struct list_elem* elem = farea[order].pg_list.head;
    while (elem && elem < pg) {
        elem = elem->next;
    }
    if (elem == NULL) {
        list_push_back(&farea[order].pg_list, &pg->lru);
    } else {
        list_insert_front(&farea[order].pg_list, elem, &pg->lru);
    }  */
}


void init()
{
    list_init(&farea[MAX_ORDER].pg_list);
    farea[MAX_ORDER].nr_free = 100;
    for (int i = 0; i < 100; i++) {
        list_push_back(&farea[MAX_ORDER].pg_list, &((mem_map + 1024 * i)->lru));
    }
   // list_push_back(&farea[MAX_ORDER].pg_list, &(mem_map->lru));
    
   // mem_map->private = MAX_ORDER;
    for (int i = 0; i < MAX_ORDER + 1; i++) {
        farea[i].bmap.bits = malloc(10000);
        bitmap_init(&farea[i].bmap, 50000);
    }
}

void print()
{
   /* for (int i = 0; i <= MAX_ORDER; i++) {
        printf("free_area %d free_cnt:%d\n", i, farea[i].nr_free);
    }*/
    for (int i = 0; i <= MAX_ORDER; i++) {
        printf("order:%d bitmap:%x\n", i, *(farea[i].bmap.bits));
    }
}
int a[100];
struct page* pp[100];
int main()
{
    init();
   
    for (int i = 0; i < 100; i++) {
        a[i] = rand() % 10;
        pp[i] = buddy_alloc(a[i]);
     //   print();
    }
   // print();
    for (int i = 0; i < 100; i++) {
        buddy_free(pp[i]);
       // print();
    }
    
    for (int i = 0; i < 100; i++) {
        pp[i] = buddy_alloc(a[i]);
    }
/*  //    pp[0] = buddy_alloc(3);
     pp[1] = buddy_alloc(6);
  //   pp[2] = buddy_alloc(7); 
     pp[3] = buddy_alloc(5); 
     pp[4] = buddy_alloc(3); 
     pp[5] = buddy_alloc(5); 
    // pp[6] = buddy_alloc(6); 
 //    buddy_free(pp[0]);
     buddy_free(pp[1]);
  //   buddy_free(pp[2]);
     buddy_free(pp[3]);
     buddy_free(pp[4]);
     buddy_free(pp[5]);
   //  buddy_free(pp[6]);*/
} 