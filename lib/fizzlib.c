#include "fizzlib.h"

/* 
 * 根据进制把整型转字符串并写入dest
 */
int itoa(int32_t value, char* dest, int radix) {
    int count = 0;

    if (value < 0) {
        *dest = '-';
        dest++;
        value = -value;
        count++;
    }

    if(value == 0) {
        *dest = '0';
        return 1;
    } 
    int32_t num = value;
    int idx = 0;
    
    while(num) {
        num /= radix;
        idx++;
    }
    count += idx;
    while(value) {
        idx--;
        if(value % radix >= 10) {
            *(dest + idx) = value % radix + 'a' - 10;
        } else {
            *(dest + idx) = value % radix + '0';
        }       
        value /= radix;
    }
    return count;
}

/* 
 * 根据进制把整型转字符串并写入dest
 */
int uitoa(uint32_t value, char* dest, int radix) {
    int count = 0;

    if(value == 0) {
        *dest = '0';
        return 1;
    } 
    uint32_t num = value;
    int idx = 0;
    
    while(num) {
        num /= radix;
        idx++;
    }

    count += idx;
    while(value) {
        idx--;
        if(value % radix >= 10) {
            *(dest + idx) = value % radix + 'a' - 10;
        } else {
            *(dest + idx) = value % radix + '0';
        }       
        value /= radix;
    }
    
    return count;
}