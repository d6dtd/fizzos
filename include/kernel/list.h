#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H
#include "global.h"
#include "debug.h"
struct list_elem {
    struct list_elem* prev;
    struct list_elem* next;
};

struct list {
    struct list_elem* head;
    struct list_elem* tail;
};

bool list_empty(struct list* list);
void list_init(struct list* list);
void list_insert_back(struct list* list, struct list_elem* before_elem, struct list_elem* elem);/* TODO */
void list_insert_front(struct list* list, struct list_elem* back_elem, struct list_elem* elem);
void list_delete(struct list* list, struct list_elem* elem);
void list_push_back(struct list* list, struct list_elem* elem);
/* 返回最后一个 */
struct list_elem* list_pop_back(struct list* list);
/* 返回第一个 */
struct list_elem* list_pop_front(struct list* list);
struct list_elem* list_front(struct list* list);
struct list_elem* list_back(struct list* list);
void list_forward(struct list* list, struct list_elem* elem);
void list_backward(struct list* list, struct list_elem* elem);
bool list_find(struct list* list, struct list_elem* elem);
void list_traversal(struct list *list, bool (*f)(struct list_elem *elem, void *arg), void *arg);
void list_push_front(struct list *list, struct list_elem* elem);
void insert_in_order(struct list* list, struct list_elem* elem,\
                         uint32_t offset, bool flag);
#endif