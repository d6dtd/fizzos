#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define bool int
#define true 1
#define false 0
#define HEIGHT 25
#define WIDTH  80   /* TODO 以后可以改一下尺寸 */

#define MAX_PATH_LEN    64
#define MAX_CMD_LEN     128

char screen[HEIGHT][WIDTH];

int head_row;   // 屏幕最上面一行在二维数组中的索引

char current_path[MAX_PATH_LEN + 1] = {'/'};

static void print_path() {
    printf("d6dtd@fizzos:%s$ ", current_path);
    fflush(stdout);
}

static char cmd_buf[MAX_CMD_LEN + 1]; // 留个字符结尾
static int cmd_pos = 0;

static char buf[100];
static char* args[3];  // 最多三个参数

static void parse_cmd()
{
    char* cmd = cmd_buf;
    char* arg = strchr(cmd_buf, ' ');
    int idx = 0;
    while (arg) {
        *arg = 0;
        arg++;
        args[idx] = arg;
        arg = strchr(arg, ' ');
        idx++;
    }
    if (strcmp(cmd, "cd") == 0) {
        printf("args[0]:%s\n", args[0]);
        chdir(args[0]); // TODO 错误时打印，调用(str)error系统调用
        getcwd(current_path, MAX_PATH_LEN + 1);
    } else if (strcmp(cmd, "pwd") == 0) {
        getcwd(buf, 100);
        printf("%s\n", buf);
    } else {
        printf("unknown command\n");
    }
}

void shell_proc()
{
    while(1) {
        bool is_done = false;
        print_path();
        int count = read(0, cmd_buf + cmd_pos, MAX_CMD_LEN);
        printf("count:%d\n", count);
        cmd_buf[count - 1] = 0; // 去掉换行符
        parse_cmd();
        memset(cmd_buf, 0, MAX_CMD_LEN);
    }
}
int main(){
	shell_proc();
}
