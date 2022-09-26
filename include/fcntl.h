#ifndef __FCNTL_H
#define __FCNTL_H

/* 用户和内核共用的可以放在这里 */
#define O_RDONLY	     00
#define O_WRONLY	     01
#define O_RDWR		     02
#define O_CREAT	   0100	/* Not fcntl.  */
#define O_EXCL		   0200	/* Not fcntl.  */
#define O_NOCTTY	   0400	/* Not fcntl.  */
#define O_TRUNC	  01000	/* Not fcntl.  */
#define O_APPEND	  02000
#define O_NONBLOCK	  04000

#define AT_FDCWD		-100

#endif