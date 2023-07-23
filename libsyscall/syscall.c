#include "syscall.h"

#include <sys/syscall.h>

void sys_exit(int rval) {
    __asm__ __volatile__("int $0x80" : : "a"(SYS_EXIT), "b"(rval));
}

int sys_print(const char* msg) {
    int eax;
    __asm__ __volatile__("int $0x80" : "=a"(eax) : "0"(SYS_PRINT), "b"((uint32_t)(uintptr_t) msg));
    return eax;
}