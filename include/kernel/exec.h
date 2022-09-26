#ifndef __KERNEL_EXEC_H
#define __KERNEL_EXEC_H

int do_exec(char* pathname, const char* const* argv, const char* const* envp);
int sys_execve(const char* name, const char* const* argv, const char* const* envp);
#endif