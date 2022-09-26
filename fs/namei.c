#include "namei.h"
#include "file.h"
#include "../include/kernel/memory.h"
#include "usercopy.h"
#include "fizzstr.h"
#include "dcache.h"
#include "../include/err.h"

char* getname(char* __user pathname)
{
    char* p = kmalloc(MAX_PATH_LEN + 1); // TODO 判断内存再拷贝
    strncpy_from_user(p, pathname, MAX_PATH_LEN);
    p[MAX_PATH_LEN] = 0;
    return p;
}

void putname(char* pathname)
{
    kfree(pathname);
}
/*
bool qstr_is_equal(const struct qstr* q1, const struct qstr* q2)
{
    if (q1->len != q2->len) return false;
    return strncmp(q1->name, q2->name, q1->len) == 0;
}

/* 
 * 返回最后一个目录的目录项并存到nd中
 */
/*
int path_walk(struct nameidata* nd, const char* pathname)
{
    const char* next;
    struct qstr d_name;

    while (*pathname == '/') pathname++;
    if (*pathname == '\0') return 0;
    next = pathname;

    while (*pathname) {
        
        while(*next && *next != '/') {
            next++;
        }
        d_name.name = pathname;
        d_name.len = next - pathname;

        while (*next == '/') next++; // 为了应对a/或b//这种情况
        if (*next) break;

        nd->path = search_dentry(nd->path, pathname);
        if (IS_ERR(nd->path)) {
            return PTR_ERR(nd->path);
        }
        if (nd->path->inode == NULL) {
            // 该目录不存在
            return -ENOENT;
        }

        nd->depth++;
        pathname = next;
    }
    return 0;
}*/