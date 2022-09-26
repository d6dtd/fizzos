#include "print.h"
#include "fizzarg.h"
#include "global.h"
#include "debug.h"
#include "fizzlib.h"
#include "fizzstr.h"

char console_buf[500];

// 会在size+1补一个0
int vsnprint(char* dest, size_t size, char* format, va_list ap)
{
    int source = 0;
    int dest_idx = 0;
    bool flag = false;

    int num;
    char* str;
    char c;

    while (format[source] != '\0' && dest_idx < size) {
        if(format[source] != '%' && flag == false) {
            dest[dest_idx] = format[source];
            source++;
            dest_idx++;
            continue;
        }

        if(format[source] == '%') {
            source++;
            flag = true;
            continue;
        }

        ASSERT(flag == true);
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
                        int tmp = dest_idx;
                        dest_idx += uitoa(num, dest + dest_idx, 16);
                        break;
            default:PANIC;
        }
        flag = false;
        source++;
        if(dest_idx >= 500) {
            PANIC;
        }
    }
    dest[dest_idx] = '\0';
    return dest_idx;
}

int vsprint(char* dest, char* format, va_list ap)
{
    return vsnprint(dest, -1, format, ap); // 可能越界
}

int printk(char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    return vprintk(format, ap);
}
int vprintk(char* format, va_list ap)
{
    int ret = vsprint(console_buf, format, ap);
    put_str(console_buf);
    return ret;
}

int sprintk(char* dest, char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    return vsprint(dest, format, ap);
}
int snprintk(char* dest, size_t size, char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    return vsnprint(dest, size, format, ap);
}