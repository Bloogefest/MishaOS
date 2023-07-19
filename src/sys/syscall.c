#include "syscall.h"

#include <lib/kprintf.h>

static int sys_print(const char* str) {
    if (!str) {
        return -1; // TODO: Segmentation fault
    }

    kprintf("%s", str);
    return 0;
}

static uint32_t syscalls[] = {
        (uint32_t) 0, // sys_exit
        (uint32_t) &sys_print,
};

void syscall_handle(int* eax, int ebx, int ecx, int edx, int esi, int edi) {
    static int* current_eax;

    if ((uint32_t) *eax >= sizeof(syscalls) / sizeof(syscalls[0])) {
        return;
    }

    uintptr_t handler = syscalls[(uint32_t) *eax];
    if (handler == 0) {
        return;
    }

    current_eax = eax;

    uint32_t ret;
    asm volatile("push %1\n"
                 "push %2\n"
                 "push %3\n"
                 "push %4\n"
                 "push %5\n"
                 "call *%6\n"
                 "pop %%ebx\n"
                 "pop %%ebx\n"
                 "pop %%ebx\n"
                 "pop %%ebx\n"
                 "pop %%ebx\n"
                 : "=a"(ret)
                 : "r"(edi), "r"(esi)
                 , "r"(edx), "r"(ecx)
                 , "r"(ebx), "r"(handler));

    *current_eax = ret;
}