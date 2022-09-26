
#include <unistd.h>
#include <stdio.h>
int a[100] = {1};

int uitoa(unsigned value, char* dest, int radix) {
    int count = 0;

    if(value == 0) {
        *dest = '0';
        return 1;
    } 
    unsigned num = value;
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

char s[10];
int main() {
    printf("%d\n", uitoa(0xfffff000, s, 16));
}
