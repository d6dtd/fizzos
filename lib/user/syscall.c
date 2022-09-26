#include "../../include/lib/user/syscall.h"

int errno;

_syscall0(pid_t, fork);
_syscall1(void, exit, int, status);
_syscall3(ssize_t, read, int, fd, void*, buf, size_t, count); // 3
_syscall3(ssize_t, write, int, fd, const void*, buf, size_t, count); // 4
_syscall3(ssize_t, open, const char*, pathname, int, flags, int, mode); // 5
_syscall1(int, close, int, fd); // 6
_syscall3(int, execve, const char*, pathname, const char* const*, argv, const char* const*, envp); // 11
_syscall1(int, chdir, const char*, path); // 12
_syscall0(pid_t, getpid); // 20
_syscall2(int, mkdir, const char*, pathname, mode_t, mode); // 39
_syscall2(int, getcwd, char*, buf, size_t, size); // 48
_syscall4(int, openat, int, dfd, const char*, filename, int, flags, mode_t, mode); // 243
_syscall3(int, mkdirat, int, dirfd, const char*, pathname, mode_t, mode); // 244



// 不标准的系统调用
_syscall0(int, printdir) //