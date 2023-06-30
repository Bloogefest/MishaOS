#include "panic.h"
#include "terminal.h"

void panic(const char* msg) {
    terminal_init();
//    terminal_clear_terminal();
    puts("!! KERNEL PANIC !!");
    puts(msg);
    puts("!! FISH IS NOT OPTIMIZED !!");

    for (;;) {
        asm("hlt");
    }
}