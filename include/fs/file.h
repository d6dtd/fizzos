#ifndef __FS_FILE_H
#define __FS_FILE_H
#include "fizzint.h"
#include "../kernel/atomic.h"
#include "sync.h"
#include "inode.h"
#include "./dir.h"

#define P_MAX_FILE_COUNT 	32
#define MAX_FILE_COUNT 		64
#define MAX_FILE_SIZE  		71680	// 12 * 512 + 512 / 4 * 512

#define FT_UNKNOWN 0
#define FT_NORMAL  (1<<12)
#define FT_DIR	   (2<<12)
#define FT_SCREEN 	   (3<<12)		// TODO 以后再改
#define FT_KEYBOARD   (4<<12)


/*
struct fdtable {
	unsigned int max_fds;
	struct file  **fd;      // current fd array 
	uint32_t *close_on_exec;
	uint32_t  open_fds;		// 支持打开32个文件
};*/

/*
 * Open file table structure
 */
struct files_struct {
	struct file* map_file; // 进程的映像文件
	atomic_t count;
	uint32_t close_on_exec;
	uint32_t open_fds;		// 支持打开32个文件
	struct file* fd_array[P_MAX_FILE_COUNT]; // NOTE 创建进程时清0，关闭文件时清0
};

struct fown_struct {
	rwlock_t lock;          /* protects pid, uid, euid fields */
//	struct pid *pid;	/* pid or -pgrp where SIGIO should be sent */
//	enum pid_type pid_type;	/* Kind of process group SIGIO should be sent to */
//	uid_t uid, euid;	/* uid/euid of process setting the owner */
//	int signum;		/* posix.1b rt signal to be delivered on IO */
};


struct file {
	atomic_t count; // 感觉不用
	loff_t pos;
	unsigned int f_flags;
	fmode_t	f_mode;	// 15-12 文件类型 后面是权限，对应inode的i_mode
	struct dentry* f_dentry;
	/* needed for tty driver, and maybe others */
	//void			*private_data;
	// TODO owner 
};

extern struct file file_table[MAX_FILE_COUNT];

ssize_t file_read(struct file *f, char* dest, size_t count, loff_t* pos);
ssize_t file_write(struct file *f, const char* src, size_t count, loff_t* pos);
int file_open(struct inode *i, struct file *f);
struct file* filp_openat(int dirfd, char* filename, int flags);
void file_init(void);
int alloc_fd(void);
void free_fd(int fd);
void install_fd(int fd, struct file* f);
loff_t file_pos_read(struct file* f);
int file_close(struct file* f);
#endif