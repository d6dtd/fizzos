#include "fizzstr.h"

void *memcpy(void *dest, const void *src, size_t n){
    dest = (uint8_t*)dest;
    if((uint32_t)dest >= (uint32_t)src && (uint32_t)dest < (uint32_t)src + n){
        for(; n > 0; n--){
            *((uint8_t*)dest + n - 1) = *((uint8_t*)src + n - 1);
        }
    }else{
        for(int i = 0; i < n; i++){
            *((uint8_t*)dest + i) = *((uint8_t*)src + i);
        }
    }

    return dest;
}
void *memset(void *s, int c, size_t n){
    for(int i = 0; i < n; i++) {
        *((uint8_t*)s + i) = c;
    }
    return s;
}
int memcmp(const void *s1, const void *s2, size_t n){
    int idx = 0;
    while(idx < n){
        if(*((int8_t*)s1 + idx) != *((int8_t*)s2 + idx)){
            return *((int8_t*)s1 + idx) - *((int8_t*)s2 + idx) > 0 ? 1 : -1;
        }
        idx++;
    }
    return 0;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while(*src) {
        *dest = *src;
        src++;
        dest++;
    }
    *dest = 0;
    return d;
}

char* strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for ( ; i < n; i++) // 不足的后面全0
        dest[i] = '\0';

    return dest;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while(*(s + len)) {
        len++;
        if(len >= 100) return -1; // 防止越界
    }
    return len;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 != 0 && *s2 != 0 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    while (*s1 != 0 && *s2 != 0 && *s1 == *s2 && n) {
        s1++;
        s2++;
        n--;
    }
    return *s1 - *s2;
}

char* strchr(const char *s, int c)
{
    while (*s != 0 && *s != c) {
        s++;
    }
    return *s != c ? 0 : s;
}

//搜索s1字符串，搜索的内容是s2中任意第一个出现字符
char *strpbrk(const char *s1,const char *s2)
{
    char tmp[128] = {0};
    while (*s2) {
        tmp[*s2] = 1;
        s2++;
    }
    while (*s1) {
        if (tmp[*s1]) return s1;
        s1++;
    }
    return 0;
}