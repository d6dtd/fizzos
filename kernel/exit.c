#include "exit.h"
#include "../include/kernel/sched.h"
#include "print.h"
#include "kthread.h"

static int exit_fs()
{
    // TODO fork或exec的时候，cwd的目录项引用加一
}

static int exit_file()
{
    
}

void sys_exit(int status) // 暂时不处理返回值
{
    struct task_struct* current = get_current();
    // TODO 清理除pcb之外的所有东西
    current->state = EXIT;
    current->exit_code = status;
    schedule();
}