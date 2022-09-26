#ifndef __FS_SUPER_BLOCK_H
#define __FS_SUPER_BLOCK_H
#include "fizzint.h"

struct super_block {
    uint32_t magic;                 /* 魔数 */

    uint32_t sec_cnt;               /* 本分区的扇区数 TODO 为什么在这里记录 */
    uint32_t inode_cnt;             /* inode节点个数 */
    uint32_t part_lba_base;         /* 本分区的起始扇区 */

    uint32_t block_bitmap_lba;      /* 块位图起始扇区 */
    uint32_t block_bitmap_sects;    /* 块位图占用扇区数 */

    uint32_t inode_bitmap_lba;      /* inode位图起始扇区 */
    uint32_t inode_bitmap_sects;    /* inode位图占用扇区数 */

    uint32_t inode_table_lba;       /* inode表起始扇区 */
    uint32_t inode_table_sects;     /* inode表扇区数 */

    uint32_t data_start_lba;        /* 数据的起始扇区 */
    uint32_t root_inode_no;         /* TODO 一个超级块一个根目录？一个分区只能有一个超级块吗 */
    uint32_t dir_entry_size;        /* 目录项大小 TODO 感觉可以去掉 */

    uint8_t pad[460];
}__attribute__((packed));
#endif