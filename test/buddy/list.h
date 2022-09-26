
#include "./bitmap.h"


struct list_elem {
    struct list_elem* prev;
    struct list_elem* next;
};

struct list {
    struct list_elem* head;
    struct list_elem* tail;
};

void list_init(struct list* list) {
    list->head = NULL;
    list->tail = NULL;
}
void list_insert_back(struct list* list, \
                    struct list_elem* before_elem,\
                    struct list_elem* elem) {
    elem->next = before_elem->next;
    elem->prev = before_elem;
    before_elem->next = elem;
    if(elem->next){
        elem->next->prev = elem;
    } else {
        list->tail = elem;
    }
}
void list_insert_front(struct list* list, \
                    struct list_elem* back_elem, \
                    struct list_elem* elem) {
    elem->prev = back_elem->prev;
    elem->next = back_elem;
    back_elem->prev = elem;
    if (elem->prev) {
        elem->prev->next = elem;
    } else {
        list->head = elem;
    }
}

void list_push_back(struct list* list, struct list_elem* elem) 
{
    
    
    if (list->tail == NULL) { 

        ASSERT(list->head == NULL);
        list->head = elem;
        list->tail = elem;
        elem->prev = NULL;
        elem->next = NULL;

    } else {

        list->tail->next = elem;
        elem->prev = list->tail;
        elem->next = NULL;
        list->tail = elem;
    }
}

struct list_elem* list_pop_back(struct list* list) {
    struct list_elem* elem = list->tail;
    if (list->tail == list->head) {
        list->head = NULL;
        list->tail = NULL;
    } else {
        list->tail = list->tail->prev;
    }
    return elem;
}

struct list_elem* list_front(struct list* list) {
    if(list  == NULL || list->head == NULL || list->tail == NULL) {
        PANIC; 
        return NULL;
    }
    return list->head;
}

struct list_elem* list_back(struct list* list) {
    if(list  == NULL || list->head == NULL || list->tail == NULL) {
        PANIC; 
        return NULL;
    }
    return list->tail;
}

/* 返回第一个，没有元素就返回空 */
struct list_elem* list_pop_front(struct list* list) {
    
    ASSERT(list);
    if(list->head == NULL) {
        ASSERT(list->tail == NULL);
        
        return NULL;
    }

    struct list_elem* elem = NULL;
    if (list->head == list->tail) {
        elem = list->head;
        list->head = NULL;
        list->tail = NULL;
    } else {
        elem = list->head;
        list->head = list->head->next;
        elem->next = NULL;
    }
    
    ASSERT(elem);
    ASSERT(list->tail == NULL && list->head == NULL ||\
     list->tail >= 0xc0000000 && list->head >= 0xc0000000);
    return elem;
}

bool list_empty(struct list* list) {
    return list->head == NULL;
}

/* 
 * 所在节点向前移动一位
 */
void list_forward(struct list* list, struct list_elem* elem) {
    

    ASSERT(list->head && list->tail && list->head != elem && elem->prev);
    struct list_elem* elem_prev = elem->prev;
    if (list->tail == elem) {
        list->tail = elem_prev;
    } else {
        elem->next->prev = elem_prev;
    }

    if (list->head == elem_prev) {
        list->head = elem;
    } else {
        elem_prev->prev->next = elem;
    }
    elem->prev = elem_prev->prev;
    elem_prev->next = elem->next;
    elem->next = elem_prev;
    elem_prev->prev = elem;

    
}

/* 
 * 所在节点向后移动一位
 */
void list_backward(struct list* list, struct list_elem* elem) {
    
    ASSERT(list->head && list->tail && list->tail != elem && elem->next);
    struct list_elem* elem_next = elem->next;
    if (list->tail == elem_next) {
        list->tail = elem;
    } else {
        elem_next->next->prev = elem;
    }

    if (list->head == elem) {
        list->head = elem_next;
    } else {
        elem->prev->next = elem_next;
    }

    elem->prev = elem_next->prev;
    elem_next->next = elem->next;
    elem->next = elem_next;
    elem_next->prev = elem;
    
}

void list_delete(struct list* list, struct list_elem* elem) {
    
    ASSERT(list_find(list, elem));

    if (list->head == list->tail) {

        ASSERT(list->head == elem);
        list->head = NULL;
        list->tail = NULL;

    } else if(list->head == elem) {

        list->head = elem->next;
        list->head->prev = NULL;

    } else if(list->tail == elem) {

        list->tail = elem->prev;
        list->tail->next = NULL;

    } else {
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;

        elem->prev = NULL;
        elem->next = NULL;
    }
    
}

bool list_find(struct list* list, struct list_elem* elem) {
    struct list_elem* e = list->head;

    while (e != NULL) {
        if (e == elem) {
            return true;
        }
        
        e = e->next;
    }

    return false;
}

/* 
 * 使用f函数遍历链表，f返回true时停止遍历
 */
void list_traversal(struct list *list, \
    bool (*f)(struct list_elem *elem, void *arg), void *arg)
{
    struct list_elem *head = list->head;

    while (head && !f(head, arg)) {
        head = head->next;
    }
}

void list_push_front(struct list *list, struct list_elem* elem)
{
    
    ASSERT(list && elem);
    if (list->head == NULL && list->tail == NULL) {
        list->head = elem;
        list->tail = elem;
        elem->next = NULL;
        elem->prev = NULL;
    } else {
        elem->prev = NULL;
        elem->next = list->head;
        list->head->prev = elem;
        list->head = elem;
    }
    
}

/* 
 * 按顺序插入，输入链表结点，(比较结点-链表结点)的值，flag为true则为从小到大，false为从大到小
 * NOTE: 比较的值是32位
 */
void insert_in_order(struct list* list, struct list_elem* elem,\
                         uint32_t offset, bool flag)
{
    struct list_elem* head = list->head;
        
    if (flag) {
        while (head && *(uint32_t*)((uint32_t)elem + offset) >= \
        *(uint32_t*)((uint32_t)head + offset)) {
            head = head->next;
        }
    } else {
        while (head && *(uint32_t*)((uint32_t)elem + offset) <=\
         *(uint32_t*)((uint32_t)head + offset)) {
            head = head->next;
        }      
    }
    if (head == NULL) {
        list_push_back(list, elem);
    } else {
        list_insert_front(list, head, elem);
    }
    
}