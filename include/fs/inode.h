#ifndef __FS_INODE_H
#define __FS_INODE_H
#include "fizzint.h"
#include "list.h"
#include "../kernel/atomic.h"
#include "sync.h"
#include "ide.h"

struct inode {
    uint32_t i_no;              /* 从0开始 */
    uint32_t i_flags;
    uint32_t i_size;            /* 当inode是文件时，代表文件大小，是目录时代表所有目录项大小之和 */
    atomic_t i_count;       	/* 记录此文件被打开的次数 */
    rwlock_t rwlock;			/* 读写锁 */
	umode_t  i_mode;			/* 访问权限 */
    uint32_t i_sectors[13];
    struct list_elem inode_tag;
};

// 按顺序存在磁盘上，不需要i_no 64B
struct d6dtd_inode {
    char pad[2];
	umode_t		i_mode;		/* File mode */
	uint32_t	i_size;		/* Size in bytes */
	uint32_t	i_flags;	/* File flags */
	uint32_t 	i_sectors[13];	// 12 + 512 / 4
}__attribute__((packed));

enum BITMAP_TYPE {
    BLOCK_BITMAP,
    INODE_BITMAP
};

void inode_sync(struct partition* part, struct inode* i, void* io_buf);
struct inode* inode_open(struct partition* part, uint32_t i_no);
void inode_close(struct partition* part, struct inode* inode);
struct inode* inode_alloc(struct partition* part);
void inode_delete(struct partition* part, struct inode* i);
uint32_t block_bimap_alloc(struct partition* part);
void bitmap_sync(int idx, struct partition* part, struct bitmap* bmap, enum BITMAP_TYPE btype);
int get_inode_type(struct inode* i);
#endif