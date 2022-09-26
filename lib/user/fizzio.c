#include "fizzarg.h"
#include "../../include/lib/user/syscall.h"
#include "fizzio.h"
/* 
    printf: 创建缓冲区，写入缓冲区，遇到换行write系统调用

 */

char print_buf[500];
// TODO 系统调用版的锁
// 会在size+1补一个0
int vsnprint1(char* dest, size_t size, char* format, va_list ap)
{
    int source = 0;
    int dest_idx = 0;
    int flag = 0;

    int num;
    char* str;
    char c;

    while (format[source] != '\0' && dest_idx < size) {
        if(format[source] != '%' && flag == 0) {
            dest[dest_idx] = format[source];
            source++;
            dest_idx++;
            continue;
        }

        if(format[source] == '%') {
            source++;
            flag = 1;
            continue;
        }

        switch(format[source]) {
            case 'd':   num = va_arg(ap, int);
                        dest_idx += itoa(num, dest + dest_idx, 10);
                        break;
            case 's':   str = va_arg(ap, char*);
                        strcpy(dest + dest_idx, str);
                        dest_idx += strlen(str);
                        break;
            case 'c':   c = va_arg(ap, char);
                        *(dest + dest_idx) = c;
                        dest_idx++;
                        break;
            case 'x':   num = va_arg(ap, int); // care 跨越的字节数要注意
                        dest_idx += uitoa(num, dest + dest_idx, 16);
                        break;
            default:break;
        }
        flag = 0;
        source++;
        if(dest_idx >= 500) {
            return 0;
        }
    }
    dest[dest_idx] = '\0';
    return dest_idx;
}

int vsprint1(char* dest, char* format, va_list ap)
{
    return vsnprint1(dest, -1, format, ap); // 可能越界
}

void printf(char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
}
void vprintf(char* format, va_list ap)
{
    int count = vsprint1(print_buf, format, ap);
    write(1, print_buf, count);     /* TODO 实现write系统调用 */
}

void sprintf(char* dest, char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsprint1(dest, format, ap);
}