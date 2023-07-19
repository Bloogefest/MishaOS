#include "panic.h"

#include "kprintf.h"
#include "kernel.h"

void panic(const char* msg) {
    terminal_init();
//    terminal_clear_terminal();
    puts("\n!! KERNEL PANIC !!");
    puts(msg);

    puts("\nStack Trace: ");
    struct stackframe {
        struct stackframe* ebp;
        uint32_t eip;
    };

    struct stackframe* stack;
    asm("movl %%ebp, %0" : "=r"(stack) : : );
    for (uint32_t frame = 0; stack && frame < 20; ++frame) {

        kprintf(" 0x%016lx: %s\n", stack->eip, kernel_get_func_name(stack->eip));
        stack = stack->ebp;
    }

    puts("\n!! FISH IS NOT OPTIMIZED !!");

    for (;;) {
        asm("hlt");
    }
}