#ifndef __FS_NAMEI_H
#define __FS_NAMEI_H
#define MAX_NESTED_LINKS 8
#include "global.h"
/*
struct qstr {
	// unsigned int hash; 未实现
	unsigned int len;
	const unsigned char name[MAX_FILE_NAME_LEN];
};
/*
struct nameidata {
	struct dentry   *path;
	struct qstr	last;
	struct dentry	*root;
	struct inode	*inode;
	unsigned int	flags;
	unsigned	seq;
	int		last_type;
	unsigned	depth;
	char *saved_names[MAX_NESTED_LINKS + 1];

};*/

char* getname(char* __user pathname);
void putname(char* pathname);
//bool qstr_is_equal(const struct qstr* q1, const struct qstr* q2);
#endif