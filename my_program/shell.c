#include "fizzio.h"
#include "../include/lib/user/syscall.h"
#include "fizzstr.h"

#define HEIGHT 25
#define WIDTH  80   /* TODO 以后可以改一下尺寸 */

#define MAX_PATH_LEN    64
#define MAX_CMD_LEN     128

char screen[HEIGHT][WIDTH];

int head_row;   // 屏幕最上面一行在二维数组中的索引

char current_path[MAX_PATH_LEN] = {'/'};

static void print_path() {
    printf("d6dtd@fizzos:%s$ ", current_path);
    // fflush(stdout);
}

static char cmd_buf[MAX_CMD_LEN + 1]; // 留个字符结尾
static int cmd_pos = 0;

static char buf[100];
static char* argv[11];  // 最多十个参数

/* 
 * 可以把命令按长度，字符序排序好，这样就不用一个个strcmp了
 */
static void parse_cmd()
{
    char* cmd = cmd_buf;
    char* arg = cmd_buf;
    int idx = 0;
    while (*arg) {
        argv[idx] = arg;
        idx++;
        arg = strchr(arg, ' ');
        if (!arg) break;
        *arg = 0;
        arg++;
        while (*arg && *arg == ' ') arg++;
    }
    if (idx == 0) return;
    argv[idx] = 0; // 最后一个置为0
    if (strcmp(cmd, "cd") == 0) {
        chdir(argv[1]); // TODO: 错误时打印，调用(str)error系统调用
        getcwd(current_path, MAX_CMD_LEN + 1);
    } else if (strcmp(cmd, "pwd") == 0) {
        getcwd(buf, 100);
        printf("%s\n", buf);
    } else if (strcmp(cmd, "mkdir") == 0) {
        mkdir(argv[1], 0);
    } else if (strcmp(cmd, "ls") == 0) {
        printdir();
    } else {
        sprintf(buf, "/bin/%s", cmd);
        pid_t pid = fork();
        // 子进程执行
        //while(1);
        if (pid == 0) {
            // 传命令行参数
            if (execve(buf, argv, 0) == -1) {
                printf("unknown command\n");
                exit(0);
            } else {
                printf("error !!\n");
            }
        } else {
            //print_path();
            printf("continue\n");
        }
        // TODO: wait 子进程
    }
}

int main(void)
{
    printf("main:%x\n", main);
    printf("\n");
    while(1) {
        int is_done = 0;
        //printf("print addr %x\n", print_path);
        print_path();
        int count = read(0, cmd_buf + cmd_pos, MAX_CMD_LEN);
        cmd_buf[count - 1] = 0; // 去掉换行符
        parse_cmd();
        memset(cmd_buf, 0, MAX_CMD_LEN);
    }
}