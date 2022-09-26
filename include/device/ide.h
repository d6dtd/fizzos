#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H
#include "fizzint.h"
#include "list.h"
#include "bitmap.h"
#include "sync.h"

/* 全都是存在内存中的 */

struct partition {
    uint32_t start_lba;             /* 起始扇区 */
    uint32_t sec_cnt;               /* 扇区数 */
    struct disk* my_disk;           /* 分区所属的硬盘 */
    struct list_elem part_tag;      /* 队列中的标记 */
    char name[8];                   /* 分区名称 */
    struct super_block* sb;         /* 本分区的超级块 */
    struct bitmap block_bitmap;     /* 块位图，当前一块等于一扇区 */
    struct bitmap inode_bitmap;     /* inode节点位图 */
    struct list open_inodes;        /* 已打开的inode节点队列 */
};

struct disk {
    char name[8];                       /* 本硬盘的名称 */
    struct ide_channel* my_channel;     /* 隶属于哪个通道 */
    uint8_t dev_no;                     /* 主盘0，从盘1 */
    struct partition prim_parts[4];     /* 4个主分区 */
    struct partition logic_parts[8];    /* 理论无限个，现在只支持8个 */
};

struct ide_channel {
    char name[8];               /* 本ata通道名称 */
    uint16_t port_base;         /* 本通道的起始端口号 */
    uint8_t irq_no;             /* 本通道的中断号 */
    mutex_lock_t lock;          /* 通道锁，因为一个通道有主从盘，同时只能用一个 */
    bool expecting_intr;        /* 表示正在等待硬盘的中断 */
    sem_t disk_done;            /* 用于阻塞、唤醒驱动程序 */
    struct disk devices[2];     /* 一个通道两个硬盘 */
};

extern uint8_t channel_cnt;    /* 通道数 */
extern struct ide_channel channels[2]; /* 两个ide通道 */
extern struct list partition_list;

void ide_init(void);
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt);
#endif