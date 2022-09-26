#include "print.h"
#include "init.h"
#include "../include/kernel/memory.h"
#include "interrupt.h"
#include "kthread.h"
#include "../include/device/timer.h"
#include "sync.h"
#include "ioqueue.h"
#include "../include/device/keyboard.h"
#include "console.h"
#include "process.h"
#include "../include/lib/user/syscall.h"
#include "../include/fs/fs.h"
#include "fizzio.h"
#include "../include/fcntl.h"
#include "fizzstr.h"
#include "../include/kernel/sched.h"
#include "print2.h"
#include "../include/fs/dir.h"

void uprog();

void rwlock_test ();
void slock_test();
void write_program(char* filename, uint32_t size, int start);
int kthread1(void* arg);



int main(void) // NOTE: main thread not init thread
{   
    init_all();
    
    put_str("I am kernel\n");
    //void make_init_proc();
    //make_init_proc();
    kthread_create(kthread1, NULL, 10, "1");

    
    while (1) {
        sleep(5);
    }
}

void write_program(char* filename, uint32_t size, int start)
{
    struct disk* sda = &channels[0].devices[0];
    void* buf = kmalloc(size);
    ide_read(sda, start, buf, DIV_ROUND_UP(size, 512));
    int fd = sys_open(filename, O_CREAT, 0);
    if (fd != -1) {
        sys_write(fd, buf, size);
    }
    while(1);
}

int kthread1(void* arg)
{
    //
    //chdir("d6dtd2");while(1);
    //write_program("/init", 14156, 300);
    //write_program("/bin/whoami", 14160, 300);

    //write_program("/bin/echo", 14156, 500);
   // write_program("/shell", 14544, 400);
    void make_init_proc();
    make_init_proc();
    while (1) {
        sleep(5);
    }
}

char elf[1];
void uprog()
{   
    int a = *(uint32_t*)(0xc0500000);
   // mkdir("d6dtd3", 0);
    while(1); // 进程结束没处理
}

/****************************** rwlock test****************************/
rwlock_t rwlock;
int read1(void *arg)
{
    char *s = (char *)arg;
    char a[4];
    a[3] = 0;
    while (1) {
        rwlock_read_acquire(&rwlock);
       // console_printk("acquire read lock\n");
        a[0] = s[0];
        sleep(1);
        a[1] = s[1];
        sleep(1);
        a[2] = s[2];
        rwlock_read_release(&rwlock);
       // console_printk("release read lock\n");
        console_printk("read : %s\n", a);
    }
}

int write1(void *arg)
{
    char *s = (char *)arg;
    while (1) {
        rwlock_write_acquire(&rwlock);
      //  console_printk("acquire write lock\n");
        s[0]++;
        s[1]++;
        s[2]++;
        s[3]++;
        s[4]++;
        console_printk("writing...\n");
        rwlock_write_release(&rwlock);
      //  console_printk("release write lock\n");
        sleep(5);
       // console_printk("read : %s\n", a);
    }
}

void rwlock_test ()
{
    rwlock_init(&rwlock);
    char a[5] = {'a', 'b', 'c', 'a', 'b'};
    kthread_create(read1, a, 10, "read1");
    kthread_create(read1, a + 1, 10, "read2");
    kthread_create(read1, a + 2, 10, "read3");
    kthread_create(write1, a, 10, "write1");
    while (1);
}

/****************************** rwlock test****************************/

/*******************************slock test*************************/
slock_t sl;
int slock(void* data)
{
    while (1) {
        slock_lock(&sl);
        printk("%s get lock\n", data);
        sleep(2);
        printk("%s ready unlock\n", data);
        slock_unlock(&sl);
        sleep(1);
    }
    return 0;
}

void slock_test()
{  
    slock_init(&sl);
    kthread_create(slock, (void*)"thread1", 10, "aaaaa");
    kthread_create(slock, (void*)"thread2", 10, "bbbbb");
    while(1) {
        printk("state:%d\n", sl.is_lock);
        sleep(2);
    }
}
/********************************slock test*************************/