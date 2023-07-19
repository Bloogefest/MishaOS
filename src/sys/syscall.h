#pragma once

#include <sys/isrs.h>

#define SYS_PRINT 1

void syscall_handle(int* eax, int ebx, int ecx, int edx, int esi, int edi);