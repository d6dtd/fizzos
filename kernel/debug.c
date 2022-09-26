#include "debug.h"
#include "print.h"
#include "interrupt.h"

void panic(const char* filename, int line, const char* func){
    intr_disable();
    // 因为只实现了打印16进制，以后修改一下
    uint32_t n = 0;
    int count = 0;
    while (line) {
        n = n * 16 + line % 10;
        line /= 10;
        count++;
    }

    while (count--) {
        line = line * 16 + n % 16;
        n /= 16;
    }
    
    put_str(filename);
    put_char(' ');
    put_int(line);
    put_char(' ');
    put_str(func);
    put_char(' ');
    put_str("error\n");
    while(1);
}