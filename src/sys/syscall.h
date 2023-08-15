#pragma once

#include <sys/process.h>

#define SYS_EXIT 0
#define SYS_PRINT 1
#define SYS_YIELD 2

void syscall_handle(struct syscall_regs* registers);