#include "../include/fs/dir.h"
#include "sync.h"
#include "fizzstr.h"
#include "inode.h"
#include "file.h"
#include "../include/kernel/memory.h"
#include "ide.h"
#include "super_block.h"
#include "../include/device/console.h"
#include "../include/lib/kernel/print2.h"
#include "../include/lib/fizzstr.h"
#include "../include/kernel/sched.h"
#include "usercopy.h"
#include "fcntl.h"
#include "../include/errno.h"

// 60-70字节一个对象，暂时存20个
#define MAX_D_LRU_LEN   20

struct list dentry_lru;  // 引用为0的目录项
rwlock_t lru_lock;  /* 操作链表时要加锁 */

struct dentry root_dir;

/* 
 * TODO 根据文件名找目录项 hash code
 */

/* 
 * 找目录项：在子目录项链表找，否则在磁盘找。找的过程中把目录项放到链表的首部，如果是引用的目录项就不用放到链表
 */

/* 
 * 根据目录找到对应文件名的目录项，找不到返回NULL
 * buf 至少为1024B
 */
struct dentry* search_dentry(struct partition* part, struct dentry* parent,\
                                char* dname, fname_t dname_len, void* buf)
{
    if (dname_len == 1 && *dname == '.') {
        return parent;
    } else if (dname_len == 2 && *dname == '.' && *(dname + 1) == '.') {
        return parent->d_parent; // TODO d_parent可能不在缓存中，尝试dir_open
    }
    // 先在父目录缓存中的子目录链表找
    struct list* sublist = &parent->d_subdirs;
    if (!list_empty(&sublist)) {
        struct list_elem* head = sublist->head;
        while (head != NULL) {
            struct dentry* subdir = elem2entry(struct dentry, d_child, head);
            int ret = strncmp(subdir->fname, dname, dname_len);
            if (ret == 0){
                atomic_inc(&subdir->count);
                return subdir;
            }
            head = head->next;
        }
    }

    struct inode* p = parent->inode;
    int idx = 0;
    for (idx = 0; idx < 13; idx++) {
        if (p->i_sectors[idx] == 0) continue;
        int start_lba =  p->i_sectors[idx];

        int loop = 1;
        int times = 0;
        if (idx == 12) {
            ide_read(part->my_disk, start_lba, buf + SECTOR_SIZE, 1);
            loop = SECTOR_SIZE / 4;
            start_lba = *(uint32_t*)(buf + SECTOR_SIZE);
        }
        while (times < loop) {
            ide_read(part->my_disk, start_lba, buf, 1);           
            // 不会跨扇区
            for (int j = 0; j < SECTOR_SIZE / sizeof(struct dentry_disk); j++) {
                struct dentry_disk* dentry = (struct dentry_disk*)buf + j;
                if ((dentry->i_mode & 0xf000) != FT_UNKNOWN && \
                dentry->fname_len == dname_len && \
                strncmp(dentry->fname, dname, dname_len) == 0) {
                    struct inode* i = inode_open(part, dentry->i_no);
                    struct dentry* d = kmalloc(sizeof (struct dentry));
                    memset(d, 0, sizeof (struct dentry));
                    strncpy(d->fname, dname, dname_len);
                    d->fname_len = dname_len;
                    atomic_set(&d->count, 1);
                    d->inode = i;
                    d->d_parent = parent;
                    list_push_front(&parent->d_subdirs, &d->d_child);
                    list_init(&d->d_subdirs);
                    PRINTK("d->inode no:%d d->name:%s\n", i->i_no, d->fname);
                    return d; 
                }
            }
            times++;
            start_lba = *(uint32_t*)(buf + SECTOR_SIZE + 4 * times);
        }        
    }
    return NULL;
}

void open_root_dir(struct partition* part)
{
    root_dir.inode = inode_open(part, part->sb->root_inode_no);
    root_dir.d_parent = &root_dir;
    root_dir.fname[0] = '/';
    root_dir.fname_len = 1;
}

struct dentry* dir_open(struct partition* part, uint32_t inode_no) 
{
    
}

/* 
 * 写入目录项肯定要先找到该目录，该目录肯定在缓存中，所以直接传入struct dentry十分合理
 * 写入后会放到缓存中,返回该目录项缓存中的地址
 */
struct dentry* dir_write(struct partition* part, struct dentry* parent, 
            struct inode* dentry_i, void* buf, 
            char* filename, fname_t fname_len)
{   
    struct inode* dir = parent->inode;
    if (dir->i_size == MAX_FILE_SIZE) {
        PRINTK("dir is full\n");
        return NULL;
    }

    bool is_second = false;
    for (int idx = 0; idx < 13; idx++) {
        uint32_t* next_lba =  &dir->i_sectors[idx];

        int loop = 1;
        int times = 0;
        if (idx == 12) {
            if (*next_lba == 0) {
                *next_lba = block_bimap_alloc(part);
                is_second = true;
                memset(buf, 0, SECTOR_SIZE);  // 刚分配的就不用读了，把缓冲区置零就行
            } else {
                ide_read(part->my_disk, *next_lba, buf + SECTOR_SIZE, 1);
            }
            
            loop = SECTOR_SIZE / 4;
            next_lba = buf + SECTOR_SIZE;
        }
        while (times < loop) {
            if (*next_lba == 0) {
                *next_lba = block_bimap_alloc(cur_part);
                memset(buf, 0, SECTOR_SIZE);
            } else {
                ide_read(part->my_disk, *next_lba, buf, 1);
            }
            
            // 不会跨扇区
            for (int j = 0; j < SECTOR_SIZE / sizeof(struct dentry_disk); j++) {
                struct dentry_disk* dd = (struct dentry_disk*)buf + j;
                if ((dd->i_mode & 0xf000) == FT_UNKNOWN) {
                    PRINTK("wirte dentry name:%s mode:%d i_no:%d\n", filename, dentry_i->i_mode, dentry_i->i_no);
                    dd->i_mode = dentry_i->i_mode;
                    dd->i_no = dentry_i->i_no;
                    strncpy(&dd->fname, filename, fname_len);
                    dd->fname_len = fname_len;
                    ide_write(part->my_disk, *next_lba, buf, 1); // 可能切换线程

                    if (is_second) {
                        PRINTK("dir write secondary table\n");
                        ASSERT(idx == 12);
                        ASSERT(loop > 1);
                        ide_write(part->my_disk, dir->i_sectors[12], buf + SECTOR_SIZE, 1);
                    }

                    // 如果写入的是目录，写入.和..两个目录项(感觉没什么用) TODO /* if remove */
                    if ((dentry_i->i_mode & 0xf000) == FT_DIR) {
                        dentry_i->i_sectors[0] = block_bimap_alloc(part);
                        memset(buf, 0, SECTOR_SIZE);
                        struct dentry_disk* dd2 = (struct dentry_disk*)buf;
                        dd2->fname[0] = '.';
                        dd2->fname_len = 1;
                        dd2->i_mode = dentry_i->i_mode;
                        dd2->i_no = dentry_i->i_no;
                        dd2++;
                        dd2->fname[0] = '.';
                        dd2->fname[1] = '.';
                        dd2->fname_len = 2;
                        dd2->i_mode = dir->i_mode;
                        dd2->i_no = dir->i_no;
                        dentry_i->i_size = (sizeof (struct dentry_disk)) * 2;
                        ide_write(part->my_disk, dentry_i->i_sectors[0], buf, 1);
                        PRINTK("write . and .. to lba:%d\n", dentry_i->i_sectors[0]);
                    }
                    /* 写入后顺便放在缓存中 */
                    struct dentry* dm = kmalloc(sizeof(struct dentry));
                    memset(dm, 0, sizeof (struct dentry));
                    dm->d_parent = parent;
                    strncpy(&dm->fname, filename, fname_len);
                    dm->fname_len = fname_len;
                    dm->inode = dentry_i;
                    atomic_set(&dm->count, 1);
                    list_push_front(&parent->d_subdirs, &dm->d_child);

                    dir->i_size += sizeof (struct dentry_disk);
                    return dm;
                }
            }
            times++;
            next_lba = buf + SECTOR_SIZE + 4 * times;
        }        
    }
    PANIC; // 目录满的情况已判断，不会在这里出现
    return NULL;
}

void dentry_close(struct dentry* dentry)
{
    if (dentry == &root_dir) {
        return;
    }
    ASSERT(dentry->count.counter > 0);
    if (atomic_dec_and_sete(&dentry->count)) {
        // TODO 添加到LRU链表上,现在暂时将它释放
        PRINTK("dentry %s close\n", dentry->fname);
        //inode_close(cur_part, dentry->inode); 目录项关闭 inode再关闭
       // list_push_front(&dentry_lru, &dentry->d_lru);
    }
}

/* 
 * buf 1024B
 * 相对parent的路径，返回：
 * parent：child的上一级目录
 * child：当前目录项
 * curdir: 当前文件名
 * nextdir: 下一级文件名
 */
int parse_path(int dirfd, char* path, struct search_res* res, void* buf)
{  
    PRINTK("parse_path:%s\n", path);

    struct task_struct* t = get_current();
    char *cur = path, *next = cur;
    int curlen = 0;
    struct dentry* parent;
    struct dentry *child;
    
    if (path[0] == '/') {
        parent = t->fs->root;
        cur++;
        next++;
    } else if (dirfd == AT_FDCWD) {
        parent = t->fs->cwd;
    } else {
        parent = t->files->fd_array[dirfd]->f_dentry;
        if (!(parent->inode->i_mode & FT_DIR)) {
            return -ENOTDIR;
        }
    }
    child = parent;
    while (next && *next && child) { //*next为了应付/或cd a/ 等情况 FIXME:输入不存在的目录卡死了 parse
        parent = child; 
        cur = next;
        next = strchr(cur, '/');      
        
        if (next != NULL) {
            curlen = next - cur;
            *next = 0;
            next++; // TODO 支持///连续斜杠
        } else {
            curlen = strlen(cur);
        }
        child = search_dentry(cur_part, parent, cur, curlen, buf);
    }
    res->parent = parent;
    res->child = child;
    res->curdir = cur;
    res->nextdir = next;
    res->curdir_len = curlen;
    return 0;
}

// 复制到缓冲区的最后(从后面往回写)
static void prepend(char **buffer, int *buflen, const char *str, int namelen)
{
    *buflen -= namelen;
    *buffer = (void*)*buffer - namelen;
    memcpy(*buffer, str, namelen);
}

void get_path(struct dentry *d, char **buffer, int *buflen)
{
    if (d->d_parent == d) {
        prepend(buffer, buflen, "/", 1);
        return;
    }

    while (d->d_parent != d) {
        prepend(buffer, buflen, d->fname, d->fname_len);
        prepend(buffer, buflen, "/", 1);
        d = d->d_parent;
    }
}

// 返回写入的长度，包括'\0'的长度
int sys_getcwd(char* __user buf, size_t size)
{
    char* path = kmalloc(MAX_PATH_LEN + 1);
    struct task_struct* current = get_current();
    struct dentry *root = current->fs->root, *cwd = current->fs->cwd;
    // TODO 暂时不用到root
    void* pbuf = path + MAX_PATH_LEN + 1;
    int len = MAX_PATH_LEN + 1;
    prepend(&pbuf, &len, "\0", 1);  // 写入'\0'
    get_path(cwd, &pbuf, &len);
    PRINTK("cwd path:%s\n", pbuf);
    len = MAX_PATH_LEN + 1 - len;

    if (size < len) {
        len = size;
        ((char*)pbuf)[len - 1] = '\0';
    }

    copy_to_user(buf, pbuf, len);
    kfree(path);
    return len;
}

/* 是否需要检测同名？ */
int dir_create(struct partition* part, struct dentry* parent,\
                 mode_t mode, void* buf, char* fname, fname_t fname_len)
{
    struct inode* i = inode_alloc(cur_part);
    i->i_mode = (mode & 0x0fff) | FT_DIR;
    struct dentry* dentry;
    if (dentry = dir_write(cur_part, parent, i, buf, fname, fname_len)) {
        dentry->inode = i;
        atomic_inc(&i->i_count); // 先引用一下，dentry暂时放在缓存中
        inode_sync(part, i, buf);
        bitmap_sync(i->i_no, part, &part->inode_bitmap, INODE_BITMAP);
        return 0;
    } else {
        inode_delete(cur_part, i);
        return -1;
    }
}

int sys_mkdirat(int dirfd, const char *pathname, mode_t mode)
{
    struct task_struct* t = get_current();
    struct dentry* d;
    // TODO 拷贝pathname
    char* path = kmalloc(MAX_PATH_LEN + 1);
    int len = strncpy_from_user(path, pathname, MAX_PATH_LEN);  
    path[len] = 0;

    if (path[0] == '/') {
        PRINTK("mkdir relative of root\n");
        d = t->fs->root;
    } else if (dirfd == AT_FDCWD) {
        PRINTK("mkdir relative of cwd\n");
        d = t->fs->cwd;
    } else {
        ASSERT(t->files->open_fds & (1 << dirfd)); // TODO 以后处理, 凡是传fd进来的都要处理
        PANIC;
       // d = t->files->fd_array[dirfd]; // TODO 看看file结构体有无dentry
    }

    void* buf = kmalloc(1024);
    struct search_res res;
    parse_path(dirfd, path, &res, buf);
    ASSERT(res.child == NULL); // TODO 处理已创建的情况
    // res.curdir存的是path那块内存的指针，不用担心指向已分配的内存
    
    int ret = dir_create(cur_part, res.parent, mode, buf, res.curdir, res.curdir_len);

    kfree(buf);
    kfree(path);
    return ret;
}

int sys_mkdir(const char *pathname, mode_t mode)
{
    return sys_mkdirat(AT_FDCWD, pathname, mode);
}

int sys_chdir(const char* pathname)
{
    char* path = kmalloc(MAX_PATH_LEN + 1);
    int len = strncpy_from_user(path, pathname, MAX_PATH_LEN);  
    path[len] = 0; 
    int ret;
    void* buf = kmalloc(1024);

    struct search_res res;
    parse_path(AT_FDCWD, pathname, &res, buf);

    if (res.child == NULL) {
        PRINTK("sys_chdir error\n");
        ret = -1;
    } else {
        ASSERT(res.child->inode->i_mode & FT_DIR);
        get_current()->fs->cwd = res.child;
        ret = 0;
    }

    kfree(path);
    kfree(buf);
    return ret;
}

/*
 * 搜索
 * bool func(void*) 返回true则继续搜索
 * */
void dir_search_disk(struct partition* part, struct inode* p, bool func(struct dentry_disk*, void*), void* arg) {
    ASSERT(get_inode_type(p) == FT_DIR);

    void* buf = kmalloc(1024);
    for (int idx = 0; idx < 13; idx++) {
        if (p->i_sectors[idx] == 0) continue;
        int start_lba =  p->i_sectors[idx];

        int loop = 1;
        int times = 0;
        if (idx == 12) {
            ide_read(part->my_disk, start_lba, buf + SECTOR_SIZE, 1);
            loop = SECTOR_SIZE / 4;
            start_lba = *(uint32_t*)(buf + SECTOR_SIZE);
        }
        while (times < loop) {
            ide_read(part->my_disk, start_lba, buf, 1);
            // 不会跨扇区
            for (int j = 0; j < SECTOR_SIZE / sizeof(struct dentry_disk); j++) {
                struct dentry_disk* dentry = (struct dentry_disk*)buf + j;
                if ((dentry->i_mode & 0xf000) != FT_UNKNOWN) {
                    if (func(dentry, arg) == false) break;
                }
            }
            times++;
            start_lba = *(uint32_t*)(buf + SECTOR_SIZE + 4 * times);
        }
    }
    kfree(buf);
}

static struct dentrys {
    struct dentry_disk disks;
    struct list_elem elem;
};

static bool add_dentry(struct dentry_disk* dentry, void* arg) {
    struct list* dentry_list = (struct list*)arg;
    struct dentrys* ds = kmalloc(sizeof (struct dentrys));
    ds->disks = *dentry;
    list_push_back(dentry_list, &ds->elem);
    return true;
}


int sys_printdir() {
    struct task_struct* current = get_current();
    struct dentry* dir = current->fs->cwd;
    struct inode* p = dir->inode;

    struct list dentry_list;
    list_init(&dentry_list);

    dir_search_disk(cur_part, p, add_dentry, (void*)&dentry_list);

    int c = 0;

    char buf[MAX_FILE_NAME_LEN + 10];
    while (!list_empty(&dentry_list)) {
        struct list_elem* elem = list_pop_front(&dentry_list);
        struct dentrys* ds = elem2entry(struct dentrys, elem, elem);

        memset(buf, 0, MAX_FILE_NAME_LEN + 10);
        memcpy(buf, ds->disks.fname, ds->disks.fname_len);
        for (int i = ds->disks.fname_len; i < MAX_FILE_NAME_LEN; i++) {
            buf[i] = ' ';
        }

        int type = ds->disks.i_mode & 0xf000;
        if (type == FT_NORMAL) {
            memcpy(buf + MAX_FILE_NAME_LEN, "normal\n", 7);
        } else if (type == FT_DIR) {
            memcpy(buf + MAX_FILE_NAME_LEN, "dir\n", 4);
        }
        console_printk(buf);
        kfree(ds);
        c++;
    }
    return c;
}