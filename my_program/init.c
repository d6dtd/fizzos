#include "fizzio.h"
#include "../include/lib/user/syscall.h"

int main(void)
{   
    int pid = fork();
    if (pid > 0) {
        printf("I am parent pid:%d %d\n", getpid(), pid);
        exit(0);
    } else {
        printf("I am children pid:%d %d\n", getpid(), pid);
        execve("shell", 0, 0);
        printf("should be not there\n");
    }
}