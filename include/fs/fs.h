#ifndef __FS_FS_H
#define __FS_FS_H
#include "fizzint.h"
#include "global.h"

#define MAX_FILES_PER_PART  4096    /* 每个分区所支持的最大文件数 */
#define BITS_PER_SECTOR     4096    /* 每扇区的位数 */
#define SECTOR_SIZE         512     /* 扇区字节大小 */
#define BLOCK_SIZE  SECTOR_SIZE     /* 块字节大小 */

extern struct partition *cur_part;

void filesys_init(void);
ssize_t sys_write(int fd, const void *buf, size_t count);
ssize_t sys_read(int fd, void *buf, size_t count);
int sys_open(const char* __user filename, int flags, int mode);
int sys_openat(int dirfd, const char* __user pathname, int flags, int mode);
int sys_close(int fd);
#endif