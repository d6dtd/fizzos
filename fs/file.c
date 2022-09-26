#include "../include/fs/file.h"
#include "bitmap.h"
#include "../include/kernel/memory.h"
#include "../include/kernel/sched.h"
#include "console.h"
#include "print.h"
#include "../include/device/keyboard.h"
#include "fizzstr.h"
#include "dir.h"
#include "../include/fcntl.h"
#include "../include/errno.h"
#include "../include/err.h"

struct file file_table[MAX_FILE_COUNT];

struct file* get_free_file_of_table() {
    for (int i = 0; i < MAX_FILE_COUNT; i++) {
        if (atomic_read(&file_table[i].count) == 0) {
           // printk("get file table idx:%d\n", i);
            return &file_table[i];
        }
    }
    return NULL;
}

// 系统调用期间分配的，不需要互斥锁，只需要自旋锁，单CPU没用
int alloc_fd()
{
    struct task_struct* current = get_current();
    uint32_t open_fds = current->files->open_fds;
    for (int i = 0; i < 32; i++) {
        if (!((1<<i) & open_fds)) {
            current->files->open_fds |= (1 << i);
            PRINTK("alloc fd:%d\n", i);
            return i;
        }
    }
    PRINTK("alloc fd fail\n");
    return -1;
}

void free_fd(int fd)
{
    ASSERT(fd >= 0);
    struct task_struct* current = get_current();
    ASSERT(current->files->open_fds & (1 << fd));
    current->files->open_fds &= ~((uint32_t)(1 << fd));
}

/* 
 * 当前进程安装全局文件表索引为global_idx的文件，成功返回
 * 文件描述符，失败返回-1
 */
void install_fd(int fd, struct file* f) {
    struct task_struct* current = get_current();
    struct files_struct* files = current->files;
    files->fd_array[fd] = f;
}

void file_init()
{
    memset(file_table, 0, sizeof file_table);   
    file_table[0].f_mode = FT_KEYBOARD;
    atomic_inc(&file_table[0].count);
    file_table[1].f_mode = FT_SCREEN;
    atomic_inc(&file_table[1].count);
}

/* 
 * 
 */
ssize_t file_write(struct file *f, const char* src, size_t count, loff_t* pos)
{
    if ((f->f_mode & 0xf000) == FT_SCREEN) {
        return console_print(src, count);            
    }

    loff_t p = *pos;
    void* io_buf = kmalloc(1024);
    struct inode* i = f->f_dentry->inode;

    int ret = count;

    int sec = p / SECTOR_SIZE;
    int offset = p % SECTOR_SIZE;
    uint32_t* next_lba = io_buf + 512;
    bool is_sync_inode = false;     // 同步inode
    bool is_sync_second = false;    // 同步二级索引表
    bool is_sync_block = true;      // 同步块位图

    // 起始扇区大于12,先读入2级索引表, 如果小于等于12，下面的while循环中会读
    if (sec > 12) {
        // 起始扇区大于12，则二级索引表肯定已分配
        ASSERT(i->i_sectors[12] != 0);
        ide_read(cur_part->my_disk, i->i_sectors[12], io_buf + 512, 1);
    }

    while (count > 0 && sec < 12 + 128) { // TODO 感觉应该在file结构体有part属性
        if (sec < 12) {
            next_lba = &(i->i_sectors[sec]);
        } else if (sec == 12){
            if (i->i_sectors[12] == 0) {
                i->i_sectors[12] = block_bimap_alloc(cur_part);
                ide_read(cur_part->my_disk, i->i_sectors[12], io_buf + 512, 1);
                memset(io_buf + 512, 0, 512); // 磁盘可能有脏数据
                is_sync_second = true;
            } else {
                ide_read(cur_part->my_disk, i->i_sectors[12], io_buf + 512, 1);
            }
            next_lba = io_buf + 512;
        } else {
            next_lba = io_buf + 512 + (sec - 12) * 4;
        }

        if (*next_lba == 0) {
            is_sync_second |= sec > 12;
            *next_lba = block_bimap_alloc(cur_part); // TODO 同步bitmap
        }

        int c = MIN(count, SECTOR_SIZE - offset);
        ASSERT(*next_lba != 0); // 肯定是已分配的扇区
        ide_read(cur_part->my_disk, *next_lba, io_buf, 1);
        memcpy(io_buf + offset, src, c);
        ide_write(cur_part->my_disk, *next_lba, io_buf, 1);
        count -= c;
        src += c;
        offset = 0;
        sec++;
    } 

    ASSERT(count == 0); // 写满了后面的可以截断
    *pos += ret;
    if (i->i_size < p + ret) {
        i->i_size = p + ret;
        is_sync_inode = true;
    }

    if (is_sync_second) {
        PRINTK("sync second\n");
        ide_write(cur_part->my_disk, i->i_sectors[12], io_buf + 512, 1);
    }

    if (is_sync_inode) { // 改变其他属性也可能需要同步inode
        PRINTK("sync inode\n");

        inode_sync(cur_part, i, io_buf);
    }

    kfree(io_buf);
    return ret;  
}

/*
 *  
 */
ssize_t file_read(struct file *f, char* dest, size_t count, loff_t* pos)
{
    if ((f->f_mode & 0xf000) == FT_KEYBOARD) {
       // printk("read keyboard\n");
        return keyboard_readline(dest, count);            
    }

    loff_t p = *pos;
    void* io_buf = kmalloc(1024);
    struct inode* i = f->f_dentry->inode;

    ASSERT(i->i_size >= p);
    count = MIN(i->i_size, p + count) - p;
    int ret = count;

    int sec = p / SECTOR_SIZE;
    int offset = p % SECTOR_SIZE;
    uint32_t* next_lba = io_buf + 512;

    // 起始扇区大于12,先读入2级索引表, 如果小于等于12，下面的while循环中会读
    if (sec > 12) {
        ide_read(cur_part->my_disk, i->i_sectors[12], io_buf + 512, 1);
    }

    while (count > 0 && sec < 12 + 128) { // TODO 感觉应该在file结构体有part属性
        if (sec < 12) {
            *next_lba = i->i_sectors[sec];
        } else if (sec == 12){
            ide_read(cur_part->my_disk, i->i_sectors[12], io_buf + 512, 1);
            next_lba = io_buf + 512;
        } else {
            next_lba = io_buf + 512 + (sec - 12) * 4;
        }

        int c = MIN(count, SECTOR_SIZE - offset);
       // printk("read lba:%d\n", *next_lba);
        ASSERT(*next_lba != 0); // 肯定是已分配的扇区
        ide_read(cur_part->my_disk, *next_lba, io_buf, 1);
        memcpy(dest, io_buf + offset, c);
        count -= c;
        dest += c;
        offset = 0;
        sec++;
    } 
    ASSERT(count == 0);
    *pos += ret;
    kfree(io_buf);
    return ret;  
}

struct file* file_create(char* filename, int fname_len, struct dentry* parent, void* buf)
{
    struct file* f = NULL;
    PRINTK("file create %s\n", filename);

    struct inode* i = inode_alloc(cur_part); // TODO 设置mode flag等
    i->i_mode = FT_NORMAL;
    struct dentry* f_dentry;
    if (f_dentry = dir_write(cur_part, parent, i, buf, filename, fname_len)) {
        atomic_inc(&i->i_count);
        f = get_free_file_of_table();
        f_dentry->inode = i;
        f->pos = 0;
        f->f_dentry = f_dentry;
        atomic_inc(&f->count);

        // 如果创建成功，同步inode位图，inode表
        inode_sync(cur_part, i, buf);
        bitmap_sync(i->i_no, cur_part, &cur_part->inode_bitmap, INODE_BITMAP);
    } else {
        inode_delete(cur_part, i);
    }    
    return f;
}

/* 
 * 打开文件不一定就得新建文件，但是都要搜索路径，所以在这里创建buf
 */
struct file* filp_openat(int dirfd, char* pathname, int flags)
{   
    void* buf = kmalloc(1024);

    struct file* f = NULL;
    struct search_res res;
    
    parse_path(dirfd, pathname, &res, buf);

    // child为NULL代表无该文件, next为NULL代表搜索完毕
    if (res.child == NULL && res.nextdir == NULL && (flags & O_CREAT)) {
        PRINTK("no such file and create file %s\n", res.curdir);
        f = file_create(res.curdir, res.curdir_len, res.parent, buf);
    } else if (res.child == NULL) {
        PRINTK("no such dir\n");
        f = ERR_PTR(-ENOENT);
    } else if (res.nextdir == NULL) {
        PRINTK("found file inode_no:%d\n", res.child->inode->i_no);
        f = get_free_file_of_table();
        f->f_dentry = res.child; // TODO inode的引用次数在这里加一吗？
        atomic_inc(&f->count);
        f->pos = 0;
        f->f_flags = flags;
    } else {
        PANIC;
    }

    kfree(buf);
    return f;
}

int file_close(struct file* f)
{
    ASSERT(f->count.counter > 0);
    if (atomic_dec_and_sete(&f->count)) {
        dentry_close(f->f_dentry);
    }
    return 0;
}