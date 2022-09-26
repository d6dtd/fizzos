#include "../include/fs/inode.h"
#include "super_block.h"
#include "../include/fs/fs.h"
#include "fizzstr.h"
#include "../include/kernel/memory.h"
#include "print.h"
#include "../include/fs/file.h"

struct inode_pos {
    bool two_sec;
    uint32_t sec_no;
    int offset;
};

// 感觉用链表不加锁都挺慌

/* 获取inode在磁盘上的位置 */
static void get_inode_pos(uint32_t i_no, struct partition* part, struct inode_pos* pos)
{
    struct super_block* sb = part->sb;
    pos->sec_no = sb->inode_table_lba + i_no * sizeof (struct d6dtd_inode) / SECTOR_SIZE;
    pos->offset = i_no * sizeof (struct d6dtd_inode) % SECTOR_SIZE;
    pos->two_sec = SECTOR_SIZE - pos->offset < sizeof (struct d6dtd_inode);
}

/* 
 * 主调函数先创建好，防止最后一步才内存不足 
 * io_buf为1024B 
 */
void inode_sync(struct partition* part, struct inode* i, void* io_buf)
{
    struct inode_pos i_pos;
    get_inode_pos(i->i_no, part, &i_pos);

    struct d6dtd_inode di;
    di.i_flags = i->i_flags;
    di.i_mode = i->i_mode;
    di.i_size = i->i_size;
    PRINTK("sync inode i_no:%d i_flags:%d i_mode:%d i_size:%d sec_no:%d two_sec:%d offset:%d\n",\
     i->i_no, i->i_flags, i->i_mode, i->i_size, i_pos.sec_no, i_pos.two_sec, i_pos.offset);
    memcpy(di.i_sectors, i->i_sectors, sizeof (uint32_t) * 13);

    if (i_pos.two_sec) {
        ide_read(part->my_disk, i_pos.sec_no, io_buf, 2);
        memcpy(io_buf + i_pos.offset, &di, sizeof di);
        ide_write(part->my_disk, i_pos.sec_no, io_buf, 2);
    } else {
        ide_read(part->my_disk, i_pos.sec_no, io_buf, 1);
        memcpy(io_buf + i_pos.offset, &di, sizeof di);
        ide_write(part->my_disk, i_pos.sec_no, io_buf, 1);
    }
    PRINTK("sync inode table done\n");
}

/* 
 * 根据结点号返回相应的inode结构体
 * 必须是已分配的inode
 */
struct inode* inode_open(struct partition* part, uint32_t i_no)
{
    ASSERT(i_no < 4096);
    ASSERT(bitmap_is_set(&part->inode_bitmap, i_no)); // 必须是已分配的inode

    struct inode* i;

    /* 先在open_inode链表上找 */
    struct list_elem* head = part->open_inodes.head;
    while (head != NULL) {
        i = elem2entry(struct inode, inode_tag, head);
        if (i->i_no == i_no) {
            atomic_inc(&i->i_count);
            return i;
        }
        head = head->next;
    }

    struct inode_pos i_pos;
    get_inode_pos(i_no, part, &i_pos);

    /* 写入需要提前准备缓冲区，打开不用 */
    void* buf = kmalloc(SECTOR_SIZE * 2);
    if (i_pos.two_sec) {
        ide_read(part->my_disk, i_pos.sec_no, buf, 2);
    } else {
        ide_read(part->my_disk, i_pos.sec_no, buf, 1);
    }

    i = kmalloc(sizeof(struct inode));
    struct d6dtd_inode di;

    memcpy(&di, buf + i_pos.offset, sizeof(struct d6dtd_inode));

    // 共有的属性
    i->i_flags = di.i_flags;
    i->i_mode  = di.i_mode;
    memcpy(i->i_sectors, &di.i_sectors, sizeof (uint32_t) * 13);
    i->i_size = di.i_size;
    PRINTK("open i_no:%d i_mode:%d i_size:%d\n", i_no, di.i_mode, di.i_size);

    // 内存inode特有的属性
    atomic_set(&i->i_count, 1);
    i->i_no = i_no;
    rwlock_init(&i->rwlock);

    // TODO 假如同时打开，都卡在这一步，直接GG
    // 可能马上用到，放在最前面方便检索
    list_push_front(&part->open_inodes, &i->inode_tag);
    kfree(buf);
    return i;
}

// 在close时同步磁盘
void inode_close(struct partition* part, struct inode* inode)
{
    // jian 1之后要马上删除
    if (!atomic_dec_and_setg(&inode->i_count)) {
        PRINTK("inode %d close\n", inode->i_no);
        list_delete(&part->open_inodes, &inode->inode_tag);
        kfree(inode);
    }
}

/* 
 * 分配一个inode,并设置i_no,没有同步到磁盘
 * 并且添加进open_inodes链表中
 * i->i_count为0
 */
struct inode* inode_alloc(struct partition* part)
{
    struct inode* i = kzalloc(sizeof (struct inode));
    i->i_no = bitmap_scan_set(&part->inode_bitmap);
    PRINTK("sizeof i->i_sectors = %d\n", sizeof (i->i_sectors));
    ASSERT(i->i_no >= 0);
    PRINTK("alloc inode %d\n", i->i_no);
    list_push_front(&part->open_inodes, &i->inode_tag);
    return i;
}

/* 
 * 删除一个inode,没有同步到磁盘
 * 在open_inodes链表中删除
 */
void inode_delete(struct partition* part, struct inode* i)
{
    ASSERT(i->i_no >= 0 && atomic_read(&i->i_count) == 0); // 引用必须为0
    list_delete(&part->open_inodes, &i->inode_tag);
    bitmap_reset(&part->inode_bitmap, i->i_no);
    kfree(i);
    PRINTK("delete inode %d\n", i->i_no);
}

// 返回一个空闲块的lba地址,分配马上同步到磁盘
uint32_t block_bimap_alloc(struct partition* part)
{
    uint32_t idx = bitmap_scan_set(&part->block_bitmap);
    ASSERT(idx >= 0);
    bitmap_sync(idx, part, &part->block_bitmap, BLOCK_BITMAP);
    PRINTK("alloc block idx:%d start_lba:%d\n", idx, part->sb->data_start_lba + idx);
    return part->sb->data_start_lba + idx;
}

/* 
 * 根据索引同步位图到磁盘
 * io_buf一个块
 */
void bitmap_sync(int idx, struct partition* part, struct bitmap* bmap, enum BITMAP_TYPE btype)
{
    uint32_t start_sec;
    if (btype == BLOCK_BITMAP) {
        PRINTK("sync block_bitmap idx:%d\n", idx);
        start_sec = part->sb->block_bitmap_lba + idx / BITS_PER_SECTOR;
        ASSERT(idx / SECTOR_SIZE < part->sb->block_bitmap_sects);
    } else if (btype == INODE_BITMAP) {
        PRINTK("sync inode_bitmap idx:%d\n", idx);
        start_sec = part->sb->inode_bitmap_lba + idx / BITS_PER_SECTOR;
        ASSERT(idx / SECTOR_SIZE < part->sb->inode_bitmap_sects);
    }

    // 读出的时候以扇区为单位，所以写入不会越界
    uint8_t* start = bmap->bits + (idx - idx % BITS_PER_SECTOR) / 8;
    mutex_acquire(&bmap->lock);
    ide_write(part->my_disk, start_sec, start, 1);
    mutex_release(&bmap->lock);
}

int get_inode_type(struct inode* i)
{
    return i->i_mode & 0xf000;
}