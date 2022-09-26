#include "ide.h"
#include "print.h"
#include "io.h"
#include "timer.h"
#include "debug.h"
#include "fizzstr.h"

#define reg_data(channel)   (channel->port_base + 0)
#define reg_error(channel)   (channel->port_base + 1)
#define reg_sect_cnt(channel)   (channel->port_base + 2)
#define reg_lba_l(channel)   (channel->port_base + 3)
#define reg_lba_m(channel)   (channel->port_base + 4)
#define reg_lba_h(channel)   (channel->port_base + 5)
#define reg_dev(channel)   (channel->port_base + 6)
#define reg_status(channel)   (channel->port_base + 7)
#define reg_cmd(channel)   (channel->port_base + 7)
#define reg_alt_status(channel)   (channel->port_base + 0x206)
#define reg_ctl(channel)   (channel->port_base + 0x206)

/* status寄存器的关键位 */
#define BIT_STAT_BSY    0x80    /* 硬盘忙 */
#define BIT_STAT_DRDY   0x40    /* 驱动器准备好 */
#define BIT_STAT_DRQ    0x8     /* 数据传输准备好了 */

/* device寄存器的关键位 */
#define BIT_DEV_MBS     0xa0    /* 固定为1 */
#define BIT_DEV_LBA     0x40
#define BIT_DEV_DEV     0x10

/* 硬盘操作的指令 */
#define CMD_IDENTITY        0xec    /* identity指令 */
#define CMD_READ_SECTOR     0x20    /* 读扇区指令 */
#define CMD_WRITE_SECTOR    0x30    /* 写扇区指令 */

#define MAX_LBA ((80*1024*1024/512) - 1)

/**********初始化后从这里获取硬盘信息**********/

uint8_t channel_cnt;    /* 通道数 */
struct ide_channel channels[2]; /* 两个ide通道 */

/* 分区队列 */
struct list partition_list;

/********************************************/

/* 总扩展分区的起始lba */
int32_t ext_lba_base = 0;

/* 主分区和逻辑分区的下标 */
uint8_t p_no = 0, l_no = 0; 

/* 构建一个16字节大小的结构体,用来存分区表项 */
struct partition_table_entry {
   uint8_t  bootable;		 // 是否可引导	
   uint8_t  start_head;		 // 起始磁头号
   uint8_t  start_sec;		 // 起始扇区号
   uint8_t  start_chs;		 // 起始柱面号
   uint8_t  fs_type;		 // 分区类型
   uint8_t  end_head;		 // 结束磁头号
   uint8_t  end_sec;		 // 结束扇区号
   uint8_t  end_chs;		 // 结束柱面号
/* 更需要关注的是下面这两项 */
   uint32_t start_lba;		 // 分区起始偏移扇区
   uint32_t sec_cnt;		 // 本分区的扇区数目
} __attribute__ ((packed));	 // 保证此结构是16字节大小

/* 引导扇区,mbr或ebr所在的扇区 */
struct boot_sector {
   uint8_t  other[446];		 // 引导代码
   struct   partition_table_entry partition_table[4];       // 分区表中有4项,共64字节
   uint16_t signature;		 // 启动扇区的结束标志是0x55,0xaa,
} __attribute__ ((packed));

static void select_disk(struct disk* hd) {
    uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
    if (hd->dev_no == 1) {
        reg_device |= BIT_DEV_DEV;
    }
    outb(reg_dev(hd->my_channel), reg_device);
}

static void select_sector(struct disk* hd, uint32_t lba, uint8_t sec_cnt) {
    ASSERT(lba <= MAX_LBA);
    struct ide_channel* channel = hd->my_channel;

    outb(reg_sect_cnt(channel), sec_cnt); /* 如果sec_cnt为0，说明写入256个扇区 */

    outb(reg_lba_l(channel), lba);
    outb(reg_lba_m(channel), lba >> 8);
    outb(reg_lba_h(channel), lba >> 16);

    outb(reg_dev(channel), BIT_DEV_MBS | BIT_DEV_LBA | \
    (hd->dev_no == 1 ? BIT_DEV_DEV : 0) | (lba >> 24));
}

static void cmd_out(struct ide_channel* channel, uint8_t cmd)
{
    channel->expecting_intr = true;
    outb(reg_cmd(channel), cmd);
}

static void read_from_sector(struct disk* hd, void* buf, uint8_t sec_cnt) {
    uint32_t size_in_byte;
    if (sec_cnt == 0) {
        size_in_byte = 256 * 512;
    } else {
        size_in_byte = sec_cnt * 512;
    }
    insw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

static void write2sector(struct disk* hd, void* buf, uint8_t sec_cnt) {
    uint32_t size_in_byte;
    if (sec_cnt == 0) {
        size_in_byte = 256 * 512;
    } else {
        size_in_byte = sec_cnt * 512;
    }
    outsw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

/* Q:不用中断吗，硬等？ */
static bool busy_wait(struct disk* hd) {
    struct ide_channel* channel = hd->my_channel;
    uint16_t time_limit = 30 * 1000;
    while ((time_limit -= 10) >= 0) {
        if (!(inb(reg_status(channel)) & BIT_STAT_BSY)) {
            return (inb(reg_status(channel)) & BIT_STAT_DRQ);
        } else {
            mtime_sleep(10);
        }
    }
    return false;
}

/* 从硬盘读取sec_cnt个扇区到buf */
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {   // 此处的sec_cnt为32位大小
    if(lba > MAX_LBA) {
        PRINTK("lba overflow %x\n", lba);
        PANIC;
    }
    ASSERT(sec_cnt > 0);
    mutex_acquire (&hd->my_channel->lock);

    /* 1 先选择操作的硬盘 */
    select_disk(hd);

    uint32_t secs_op;		 // 每次操作的扇区数
    uint32_t secs_done = 0;	 // 已完成的扇区数

    /* 一次最多读256扇区，所以要分多次读 */
    while(secs_done < sec_cnt) {
        if ((secs_done + 256) <= sec_cnt) {
	        secs_op = 256;
        } else {
	        secs_op = sec_cnt - secs_done;
        }

        /* 2 写入待读入的扇区数和起始扇区号 */
        select_sector(hd, lba + secs_done, secs_op);

        /* 3 执行的命令写入reg_cmd寄存器 */
        cmd_out(hd->my_channel, CMD_READ_SECTOR);	      // 准备开始读数据

        /*********************   阻塞自己的时机  ***********************
            在硬盘已经开始工作(开始在内部读数据或写数据)后才能阻塞自己,现在硬盘已经开始忙了,
            将自己阻塞,等待硬盘完成读操作后通过中断处理程序唤醒自己*/
        sema_wait(&hd->my_channel->disk_done);
        /*************************************************************/

        /* 4 检测硬盘状态是否可读 */
        /* 醒来后开始执行下面代码*/
        if (!busy_wait(hd)) {			      // 若失败
	        char error[64];
	        sprintk(error, "%s read sector %d failed!!!!!!\n", hd->name, lba);
	        PANIC;
        }

        /* 5 把数据从硬盘的缓冲区中读出 */
        read_from_sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);
        secs_done += secs_op;
    }
    mutex_release(&hd->my_channel->lock);
}

/* 将buf中sec_cnt扇区数据写入硬盘 */
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
    ASSERT(lba <= MAX_LBA);
    ASSERT(sec_cnt > 0);
    mutex_acquire (&hd->my_channel->lock);

    /* 1 先选择操作的硬盘 */
    select_disk(hd);

    uint32_t secs_op;		 // 每次操作的扇区数
    uint32_t secs_done = 0;	 // 已完成的扇区数
    while(secs_done < sec_cnt) {
        if ((secs_done + 256) <= sec_cnt) {
	        secs_op = 256;
        } else {
	        secs_op = sec_cnt - secs_done;
        }

        /* 2 写入待写入的扇区数和起始扇区号 */
        select_sector(hd, lba + secs_done, secs_op);    // 先将待读的块号lba地址和待读入的扇区数写入lba寄存器

        /* 3 执行的命令写入reg_cmd寄存器 */
        cmd_out(hd->my_channel, CMD_WRITE_SECTOR);      // 准备开始写数据

        /* 4 检测硬盘状态是否可读 */
        if (!busy_wait(hd)) {			      // 若失败
	        char error[64];
	        sprintk(error, "%s write sector %d failed!!!!!!\n", hd->name, lba);
	        PANIC;
        }
        /* 5 将数据写入硬盘 */
        write2sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);
        
        /* 在硬盘响应期间阻塞自己 */
        sema_wait(&hd->my_channel->disk_done);
        secs_done += secs_op;
    }
    /* 醒来后开始释放锁*/
    mutex_release(&hd->my_channel->lock);
}

/* 将dst中len个相邻字节交换位置后存入buf，因为identity命令返回的结果是以字为单位 */
static void swap_pairs_bytes(const char* dst, char* buf, uint32_t len)
{
   uint8_t idx;
   for (idx = 0; idx < len; idx += 2) {
      /* buf中存储dst中两相邻元素交换位置后的字符串*/
      buf[idx + 1] = *dst++;   
      buf[idx]     = *dst++;   
   }
   buf[idx] = '\0';
}

/* 获得硬盘参数信息 */
static void identify_disk(struct disk* hd) {
   char id_info[512];
   select_disk(hd);
   cmd_out(hd->my_channel, CMD_IDENTITY);
/* 向硬盘发送指令后便通过信号量阻塞自己,
 * 待硬盘处理完成后,通过中断处理程序将自己唤醒 */
   sema_wait(&hd->my_channel->disk_done);

/* 醒来后开始执行下面代码*/
   if (!busy_wait(hd)) {     //  若失败
      PANIC;
   }
   read_from_sector(hd, id_info, 1);

   char buf[64];
   uint8_t sn_start = 10 * 2, sn_len = 20, md_start = 27 * 2, md_len = 40;
   swap_pairs_bytes(&id_info[sn_start], buf, sn_len);
   PRINTK("   disk %s info:\n      SN: %s\n", hd->name, buf);
   memset(buf, 0, sizeof(buf));
   swap_pairs_bytes(&id_info[md_start], buf, md_len);
   PRINTK("      MODULE: %s\n", buf);
   uint32_t sectors = *(uint32_t *)&id_info[60 * 2];
   PRINTK("      SECTORS: %d\n", sectors);
   PRINTK("      CAPACITY: %dMB\n", sectors * 512 / 1024 / 1024);
}

/* 扫描硬盘hd中地址为ext_lba的扇区中的所有分区 */
static void partition_scan(struct disk *hd, uint32_t ext_lba)
{
    PRINTK("partition_scan from lba:%d ext_lba_base:%d\n", ext_lba, ext_lba_base);
    struct boot_sector *bs = kmalloc(sizeof(struct boot_sector));
    ide_read(hd, ext_lba, bs, 1);   /* 读引导扇区到bs指向的内存 */
    uint8_t part_idx = 0;
    struct partition_table_entry *p = bs->partition_table;

    /* 遍历分区表4个分区表项 */
    while (part_idx++ < 4)
    {
        if (p->fs_type == 0x5)
        {   
            // 若为扩展分区
            if (ext_lba_base != 0)
            {
                PRINTK("extent child part scan start\n");
                /* 子扩展分区的start_lba是相对于主引导扇区中的总扩展分区地址 */
                partition_scan(hd, p->start_lba + ext_lba_base);
            }
            else
            {   
                PRINTK("extent part scan start\n");
                // ext_lba_base为0表示是第一次读取引导块,也就是主引导记录所在的扇区
                /* 记录下扩展分区的起始lba地址,后面所有的扩展分区地址都相对于此 */
                ext_lba_base = p->start_lba;
                partition_scan(hd, p->start_lba);
            }
        }
        else if (p->fs_type != 0)
        {   
            // 若是有效的分区类型
            if (ext_lba == 0)
            {   
                // 此时全是主分区
                hd->prim_parts[p_no].start_lba = ext_lba + p->start_lba;
                PRINTK("p_no:%d start_lba:%d\n", p_no, ext_lba + p->start_lba);
                hd->prim_parts[p_no].sec_cnt = p->sec_cnt;
                hd->prim_parts[p_no].my_disk = hd;
                list_push_back(&partition_list, &hd->prim_parts[p_no].part_tag);
                sprintk(hd->prim_parts[p_no].name, "%s%d", hd->name, p_no + 1);
                p_no++;
                ASSERT(p_no < 4); // 0,1,2,3
            }
            else
            {
                hd->logic_parts[l_no].start_lba = ext_lba + p->start_lba;
                PRINTK("p_no:%d start_lba:%d\n", p_no, ext_lba + p->start_lba);
                hd->logic_parts[l_no].sec_cnt = p->sec_cnt;
                hd->logic_parts[l_no].my_disk = hd;
                list_push_back(&partition_list, &hd->logic_parts[l_no].part_tag);
                sprintk(hd->logic_parts[l_no].name, "%s%d", hd->name, l_no + 5); // 逻辑分区数字是从5开始,主分区是1～4.
                l_no++;
                if (l_no >= 8) // 只支持8个逻辑分区,避免数组越界
                    return;
            }
        }
        p++;
    }
    kfree(bs);
}

/* 打印分区信息 */
static void partition_info(struct list_elem* pelem) {
   struct partition* part = elem2entry(struct partition, part_tag, pelem);
   PRINTK("   %s start_lba:0x%x, sec_cnt:0x%x\n",part->name, part->start_lba, part->sec_cnt);
}

void intr_hd_handler(uint8_t irq_no) {
    ASSERT(irq_no == 0x2e || irq_no == 0x2f);
    uint8_t ch_no = irq_no - 0x2e;
    struct ide_channel* channel = &channels[ch_no];
    ASSERT(channel->irq_no == irq_no);

    if (channel->expecting_intr)
    {
        channel->expecting_intr = false;
        sema_post(&channel->disk_done);

        /* 读取状态寄存器使硬盘控制器认为此次的中断已被处理，从而硬盘可以继续执行新的读写 */
        inb(reg_status(channel));
    } else {
        // TODO 硬盘什么时候会发起中断
    }
}

void ide_init() {
    PRINTK("ide_init start\n");
    memset(&partition_list, 0, sizeof partition_list);
    uint8_t hd_cnt = *((uint8_t*)(0x475));  /* 获取硬盘的数量 TODO 怎么获取的 */
    ASSERT(hd_cnt > 0);
    memset(channels, 0, sizeof (channels));
    channel_cnt = DIV_ROUND_UP(hd_cnt, 2);  /* 一个通道最多两个硬盘，计算通道数 */

    struct ide_channel* channel;
    uint8_t channel_no = 0;

    while (channel_no < channel_cnt) {  /* TODO 最多只有两个通道吗？ */
        PRINTK("channel %d init\n", channel_no);
        channel = &channels[channel_no];
        sprintk(channel->name, "ide%d", channel_no);

        switch (channel_no) {
            case 0 :
                channel->port_base = 0x1f0;
                channel->irq_no = 0x20 + 14;
                break;
            case 1 :
                channel->port_base = 0x170;
                channel->irq_no = 0x20 + 15;
                break;
            default: PANIC;
        }

        channel->expecting_intr = false;
        mutex_lock_init(&channel->lock);

        sema_init(&channel->disk_done, 0); /* 中断处理程序通过post该信号量唤醒线程 */

        register_handler(channel->irq_no, intr_hd_handler);
        
        int dev_no = 0;
        while (dev_no < 2) 
        {
            struct disk* hd = &channel->devices[dev_no];
            hd->my_channel = channel;
            hd->dev_no = dev_no;
            sprintk(hd->name, "sd%c", 'a' + channel_no * 2 + dev_no);
            identify_disk(hd);
            if (dev_no != 0)
            {
                partition_scan(hd, 0);
            }
            p_no = 0, l_no = 0;
            dev_no++;
        }

        channel_no++;
    }

    struct list_elem* head = partition_list.head;
    while (head)
    {
        partition_info(head);
        head = head->next;
    }
    printk("ide_init done\n");
}