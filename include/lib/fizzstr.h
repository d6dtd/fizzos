#ifndef __LIB_STRING_H
#define __LIB_STRING_H
#include "fizzint.h"

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
char *strcpy(char *dest, const char *src);
size_t strlen(const char* s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dest, const char *src, size_t n);
char* strchr(const char *s, int c);
char *strpbrk(const char *s1,const char *s2);
#endif