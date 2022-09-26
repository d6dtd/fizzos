#ifndef __FS_DIR_H
#define __FS_DIR_H
#include "fizzint.h"
#include "list.h"
#include "./fs.h"
#include "sync.h"
#include "../kernel/atomic.h"

#define MAX_FILE_NAME_LEN 24
#define MAX_PATH_LEN    127

/* 
 * 内存中，把目录项缓存起来，目录项存在内存时，对inode的引用一直存在 
 * TODO LRU，没有实现哈希表，所以不能根据文件名获得节点，只能遍历
 */
struct dentry {
    atomic_t count;                     /* 使用计数TODO 暂时不知道有什么用 */
    struct inode* inode;                /* 该目录项的inode结点 */
    char fname[MAX_FILE_NAME_LEN + 1];
    fname_t fname_len;
    struct dentry* d_parent;     /* 父目录的目录项对象 */
    struct list_elem d_lru;             /* LRU的链表 */
    struct list_elem d_child;                /* 目录项内部形成的链表 */
    struct list d_subdirs;              /* 子目录链表 */
};

/* 磁盘的目录文件中 32B */
struct dentry_disk {
    char fname[MAX_FILE_NAME_LEN];
    fname_t fname_len;
    fmode_t i_mode; // 删除目录项后把文件类型置位 UNKNOWN
    uint32_t i_no;
}__attribute__((packed));

struct search_res {
    struct dentry* parent;
    struct dentry* child;
    char* curdir;
    fname_t curdir_len;
    char* nextdir;
};

extern struct dentry root_dir;
int parse_path(int dirfd, char* path, struct search_res* res, void* buf);
struct dentry* search_dentry(struct partition* part, struct dentry* parent, \
char* dname, fname_t dname_len, void* buf);
struct dentry* dir_write(struct partition* part, struct dentry* parent, \
    struct inode* dentry, void* buf, char* filename, fname_t fname_len);
void open_root_dir(struct partition* part);
void dentry_close(struct dentry* dentry);
int sys_getcwd(char* __user buf, size_t size);
int sys_mkdirat(int dirfd, const char *pathname, mode_t mode);
int sys_mkdir(const char *pathname, mode_t mode);
int sys_chdir(const char* pathname);
int sys_printdir();
#endif