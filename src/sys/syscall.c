#include "syscall.h"

#include <lib/kprintf.h>
#include <sys/process.h>

__attribute__((noreturn))
static int sys_exit(int rval) {
    task_exit(rval);
    while (1);
}

static int sys_print(const char* str) {
    if (!str) {
        return -1; // TODO: Segmentation fault
    }

    kprintf("%s", str);
    return 0;
}

static int sys_yield() {
    switch_task(1);
    return 0;
}

static uint32_t syscalls[] = {
        (uint32_t) &sys_exit,
        (uint32_t) &sys_print,
        (uint32_t) &sys_yield,
};

void syscall_handle(struct syscall_regs* registers) {
    if ((uint32_t) registers->eax >= sizeof(syscalls) / sizeof(syscalls[0])) {
        return;
    }

    uintptr_t handler = syscalls[(uint32_t) registers->eax];
    if (handler == 0) {
        return;
    }

    current_process->syscall_regs = registers;

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
                 : "r"(registers->edi), "r"(registers->esi)
                 , "r"(registers->edx), "r"(registers->ecx)
                 , "r"(registers->ebx), "r"(handler));

    registers = current_process->syscall_regs;
    registers->eax = ret;
}