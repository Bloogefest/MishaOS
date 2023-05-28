#include "panic.h"
#include "terminal.h"

void panic(const char* msg) {
    terminal_init();
//    terminal_clear_terminal();
    terminal_putstring("!! KERNEL PANIC !!\n");
    terminal_putstring(msg);
    terminal_putstring("\n!! FISH IS NOT OPTIMIZED !!\n");

    for (;;) {
        asm("hlt");
    }
}