#pragma once

#include <stdint.h>
#include <stddef.h>

size_t strlen(const char* str);
void* memset(void* data, int value, size_t size);
char* strstr(const char* a, const char* b);
char* _strstr(const char* a, const char* b, size_t limit);
int strcmp(const char* a, const char* b);
int _strcmp(const char* a, const char* b, size_t size);
void* memcpy(void* dst, void* src, size_t n);