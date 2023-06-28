#pragma once

#include <stddef.h>
#include <stdint.h>

int toupper(int c);
size_t strlen(const char* str);
void* memset(void* ptr, int value, size_t size);
char* strstr(const char* a, const char* b);
char* _strstr(const char* a, const char* b, size_t limit);
int memcmp(void* ptr1, void* ptr2, size_t size);
char* _strchr(const char* str, int chr, uint32_t limit);
char* strchr(const char* str, int chr);
int strcmp(const char* str1, const char* str2);
void* memcpy(void* dst, const void* src, size_t n);
size_t memrev(char* buf, size_t n);
size_t strrev(char* s);
char* itoa(int32_t target, char* buf, uint32_t radix);
char* utoa(int32_t target, char* buf, uint32_t radix);
int32_t atoi(const char* buf, uint32_t radix);