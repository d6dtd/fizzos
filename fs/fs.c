#include "../include/fs/fs.h"
#include "ide.h"
#include "dir.h"
#include "super_block.h"
#include "inode.h"
#include "global.h"
#include "fizzstr.h"
#include "../include/kernel/sched.h"
#include "file.h"
#include "../include/errno.h"
#include "namei.h"
#include "../include/err.h"
#include "../include/fcntl.h"

//#define printk printk

#define FIZZ_FILE_SYSTEM 0x20000504

struct partition *cur_part;

static bool mount_partition(struct list_elem *pelem, int arg)
{
   char *part_name = (char *)arg;
   struct partition *part = elem2entry(struct partition, part_tag, pelem);
   if (!strcmp(part->name, part_name)) {
      cur_part = part;
      struct disk *hd = cur_part->my_disk;

      struct super_block* sb_buf = (struct super_block *)kmalloc(SECTOR_SIZE);

      cur_part->sb = (struct super_block *)kmalloc(sizeof (struct super_block));

      memset(sb_buf, 0, SECTOR_SIZE);
      ide_read(hd, cur_part->start_lba + 1, sb_buf, 1);
      memcpy(cur_part->sb, sb_buf, sizeof (struct super_block));

      cur_part->block_bitmap.bits = (uint8_t *)kmalloc(sb_buf->block_bitmap_sects * SECTOR_SIZE);
      cur_part->block_bitmap.byte_size = sb_buf->block_bitmap_sects * SECTOR_SIZE;
      ide_read(hd, sb_buf->block_bitmap_lba, cur_part->block_bitmap.bits, sb_buf->block_bitmap_sects);

      cur_part->inode_bitmap.bits = (uint8_t *)kmalloc(sb_buf->inode_bitmap_sects * SECTOR_SIZE);
      cur_part->inode_bitmap.byte_size = sb_buf->inode_bitmap_sects * SECTOR_SIZE;
      ide_read(hd, sb_buf->inode_bitmap_lba, cur_part->inode_bitmap.bits, sb_buf->inode_bitmap_sects);

      list_init(&cur_part->open_inodes);
      PRINTK("mount %s done\n", part->name);

      return true;
   }

   return false;
}

/* 
 * 格式化分区，根据读出的磁盘信息，创建超级块，块位图，inode位图，inode表，根目录并写入
 * 下次运行时，如果看到分区的魔数符合，就不用再调用该函数
 */
static void partition_format(struct partition *part)
{
   uint32_t boot_sector_sects = 1;
   uint32_t super_block_sects = 1;
   uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);
   uint32_t inode_table_sects = DIV_ROUND_UP(MAX_FILES_PER_PART * sizeof(struct d6dtd_inode), SECTOR_SIZE);

   uint32_t used_sects = boot_sector_sects + super_block_sects +
                         inode_bitmap_sects + inode_table_sects;
   uint32_t free_sects = part->sec_cnt - used_sects;

   uint32_t block_bitmap_sects;
   block_bitmap_sects = DIV_ROUND_UP(free_sects, BITS_PER_SECTOR);
   uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects;
   block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR);

   struct super_block sb;
   sb.magic = FIZZ_FILE_SYSTEM; 
   sb.sec_cnt = part->sec_cnt;
   sb.inode_cnt = MAX_FILES_PER_PART;
   sb.part_lba_base = part->start_lba;

   sb.block_bitmap_lba = sb.part_lba_base + 2;
   sb.block_bitmap_sects = block_bitmap_sects;

   sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_sects;
   sb.inode_bitmap_sects = inode_bitmap_sects;

   sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_sects;
   sb.inode_table_sects = inode_table_sects;

   sb.data_start_lba = sb.inode_table_lba + sb.inode_table_sects;
   sb.root_inode_no = 0;
   sb.dir_entry_size = sizeof(struct dentry_disk);

   PRINTK("%s info:\n", part->name);
   PRINTK("   magic:0x%x\n   part_lba_base:0x%x\n   all_sectors:0x%x\n   inode_cnt:0x%x\n\
      block_bitmap_lba:0x%x\n   block_bitmap_sectors:0x%x\n   inode_bitmap_lba:0x%x\n   \
      inode_bitmap_sectors:0x%x\n   inode_table_lba:0x%x\n   inode_table_sectors:0x%x\n   \
      data_start_lba:0x%x\n",
          sb.magic, sb.part_lba_base, sb.sec_cnt, sb.inode_cnt,
          sb.block_bitmap_lba, sb.block_bitmap_sects, sb.inode_bitmap_lba, sb.inode_bitmap_sects,
          sb.inode_table_lba, sb.inode_table_sects, sb.data_start_lba);

   struct disk *hd = part->my_disk;

   /* 1 将超级块写入本分区的1扇区 */
   ide_write(hd, part->start_lba + 1, &sb, 1);
   PRINTK("   super_block_lba:0x%x\n", part->start_lba + 1);

   /* 找出数据量最大的元信息,用其尺寸做存储缓冲区*/
   uint32_t buf_size = (sb.block_bitmap_sects >= sb.inode_bitmap_sects ? sb.block_bitmap_sects : sb.inode_bitmap_sects);
   buf_size = (buf_size >= sb.inode_table_sects ? buf_size : sb.inode_table_sects) * SECTOR_SIZE;
   uint8_t *buf = (uint8_t *)kmalloc(buf_size); // 申请的内存由内存管理系统清0后返回
   memset(buf, 0, buf_size);
   /* 2 将块位图初始化并写入sb.block_bitmap_lba */
   /* 初始化块位图block_bitmap */
   buf[0] |= 0x01; // 第0个块预留给根目录,位图中先占位
   uint32_t block_bitmap_last_byte = block_bitmap_bit_len / 8;
   uint8_t block_bitmap_last_bit = block_bitmap_bit_len % 8;
   uint32_t last_size = SECTOR_SIZE - (block_bitmap_last_byte % SECTOR_SIZE); // last_size是位图所在最后一个扇区中，不足一扇区的其余部分

   
   /* 1 先将位图最后一字节到其所在的扇区的结束全置为1,即超出实际块数的部分直接置为已占用*/
   memset(&buf[block_bitmap_last_byte], 0xff, last_size);

   /* 2 再将上一步中覆盖的最后一字节内的有效位重新置0 */
   uint8_t bit_idx = 0;
   while (bit_idx <= block_bitmap_last_bit)
   {
      buf[block_bitmap_last_byte] &= ~(1 << bit_idx++);
   }
   ide_write(hd, sb.block_bitmap_lba, buf, sb.block_bitmap_sects);

   /***************************************
 * 3 将inode位图初始化并写入sb.inode_bitmap_lba *
 ***************************************/
   /* 先清空缓冲区*/
   memset(buf, 0, buf_size);
   buf[0] |= 0x1; // 第0个inode分给了根目录
   /* 由于inode_table中共4096个inode,位图inode_bitmap正好占用1扇区,
    * 即inode_bitmap_sects等于1, 所以位图中的位全都代表inode_table中的inode,
    * 无须再像block_bitmap那样单独处理最后一扇区的剩余部分,
    * inode_bitmap所在的扇区中没有多余的无效位 */
   ide_write(hd, sb.inode_bitmap_lba, buf, sb.inode_bitmap_sects);

   /***************************************
 * 4 将inode数组初始化并写入sb.inode_table_lba *
 ***************************************/
   /* 准备写inode_table中的第0项,即根目录所在的inode */
   memset(buf, 0, buf_size); // 先清空缓冲区buf
   struct d6dtd_inode *i = (struct d6dtd_inode *)buf;
   i->i_mode = FT_DIR;
   i->i_size = sb.dir_entry_size * 2;   // .和..
   i->i_sectors[0] = sb.data_start_lba; // 由于上面的memset,i_sectors数组的其它元素都初始化为0
   ide_write(hd, sb.inode_table_lba, buf, sb.inode_table_sects);

   /* 5 将根目录初始化并写入sb.data_start_lba */
   /* 写入根目录的两个目录项.和.. */
   memset(buf, 0, buf_size);
   struct dentry_disk *p_de = (struct dentry_disk *)buf;

   /* 初始化当前目录"." */
   memcpy(p_de->fname, ".", 1);
   p_de->i_no = 0;
   p_de->i_mode = FT_DIR;
   p_de->fname_len = 1;
   p_de++;

   /* 初始化当前目录父目录".." */
   memcpy(p_de->fname, "..", 2);
   p_de->i_no = 0; // 根目录的父目录依然是根目录自己
   p_de->i_mode = FT_DIR;
   p_de->fname_len = 2;
   
   /* sb.data_start_lba已经分配给了根目录,里面是根目录的目录项 */
   ide_write(hd, sb.data_start_lba, buf, 1);

   PRINTK("   root_dir_lba:0x%x\n", sb.data_start_lba);
   PRINTK("%s format done\n", part->name);
   kfree(buf);
}

/* 
 * 扫一下所有分区，读取super_block，如果发现魔数不支持，格式化分区
 */
void filesys_init()
{
   uint8_t channel_no = 0, dev_no = 0, part_idx = 0;

   struct super_block *sb_buf = (struct super_block *)kmalloc(SECTOR_SIZE);

   if (sb_buf == NULL) {
      PANIC;
   }

   PRINTK("searching filesystem......\n");
   while (channel_no < channel_cnt) {
      dev_no = 0;
      while (dev_no < 2) {
         if (dev_no == 0) {
            dev_no++; /* 跳过裸盘 */
            continue;
         }

         struct disk *hd = &channels[channel_no].devices[dev_no];
         struct partition *part = hd->prim_parts;

         while (part_idx < 12) {
            if (part_idx == 4) {
               part = hd->logic_parts;
            }

            if (part->sec_cnt != 0) {
               memset(sb_buf, 0, SECTOR_SIZE);
               PRINTK("part->start_lba:%d part_idx:%d\n", part->start_lba, part_idx);
               ide_read(hd, part->start_lba + 1, sb_buf, 1);

               if (sb_buf->magic == FIZZ_FILE_SYSTEM) {
                   PRINTK("%s has filesystem\n", part->name);
               } else {
                   PRINTK("formatting %s`s partition %s......\n", hd->name, part->name);
                  partition_format(part);
               }
            }
            part_idx++;
            part++;
         }
         dev_no++;
      }
      channel_no++;
   }

   kfree(sb_buf);

   char default_part[8] = "sdb1";
   list_traversal(&partition_list, mount_partition, default_part);
   file_init();
   open_root_dir(cur_part);
   PRINTK("part:%x data_sec_begin:%x\n", cur_part->start_lba ,cur_part->sb->data_start_lba);
   PRINTK("inode_bitmap_begin:%x\n", cur_part->sb->inode_bitmap_lba);
   PRINTK("block_bitmap_begin:%x\n", cur_part->sb->block_bitmap_lba);
}


// 完成buf长度的检验
ssize_t sys_read(int fd, void *buf, size_t count)
{
   if (fd < 0 || count < 0) return -1;
	struct task_struct* current = get_current();
   ASSERT(current->files->open_fds & (1 << fd));
   struct file* f = current->files->fd_array[fd];
	ssize_t ret = -1;
	if (f) {     
		loff_t pos = f->pos;
		ret = file_read(f, buf, count, &pos);
      //printk("read data %d bytes\n", ret);
		f->pos = pos;
	}

	return ret;
}

/* 
 * 拷贝文件名，错误就返回-1
 * 分配文件描述符，没有就返回-1
 * 打开文件，失败就释放文件描述符返回-1
 * 给文件描述符安装文件
 * 释放临时内存
 * 返回文件描述符
 */
int sys_open(const char* __user pathname, int flags, int mode)
{
   return sys_openat(AT_FDCWD, pathname, flags, mode);
}

int sys_openat(int dirfd, const char* __user pathname, int flags, int mode)
{
   // TODO 处理mode,open之后file结构体的mode可能和i_mode不一样
   char* tmp = getname(pathname);
   int fd = PTR_ERR(tmp);

   if (!IS_ERR(tmp)) {
      fd = alloc_fd();
      if (fd >= 0) {
         struct file *f = filp_openat(dirfd, tmp, flags);
         if (IS_ERR(f)) {
             PRINTK("free fd %d\n", fd);
            free_fd(fd);
            fd = PTR_ERR(f);
         } else {
             PRINTK("install fd %d\n", fd);
            install_fd(fd, f);
         }
      }    
      putname(tmp);
   }
   return fd;
}

ssize_t sys_write(int fd, const void __user *buf, size_t count)
{
   //printk("call sys_write pid:%d\n", get_current()->pid);
   if (fd < 0 || count < 0) return -1;
	struct task_struct* current = get_current();
   ASSERT(current->files->open_fds & (1 << fd));
   struct file* f = current->files->fd_array[fd];
	ssize_t ret = -1;
	if (f) {    
		loff_t pos = f->pos;
		ret = file_write(f, buf, count, &pos);
      //printk("write data %d bytes\n", ret);
		f->pos = pos;
	}

	return ret;
}

int sys_close(int fd)
{
   struct task_struct* current = get_current();
   if(fd < 0 || fd >= MAX_FILE_COUNT || !(current->files->open_fds & (1 << fd))) {
       PRINTK("error bad fd\n");
      return -EBADF;
   }
   // TODO 互斥锁

   struct file* f = current->files->fd_array[fd];
   int ret = 0;
   if((ret = file_close(f)) == 0) {
      free_fd(fd);
   }
   return ret;
}