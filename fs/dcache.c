#include "dcache.h"

/* 
 * 找不到就返回一个d_inode为NULL的目录项
 *//*
struct dentry* search_dentry(struct dentry* parent, struct qstr* name)
{
    if (*(name->name) == '.' && name->len == 1) {
        return parent;
    } else if (*(name->name) == '.' && *(name->name + 1) == '.' && name->len == 2) {
        return parent->d_parent;
    }
    
}*/