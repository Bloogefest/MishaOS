#pragma once

#include <sys/isrs.h>

#define SYS_EXIT 0
#define SYS_PRINT 1

void syscall_handle(int* eax, int ebx, int ecx, int edx, int esi, int edi);