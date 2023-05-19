#include "terminal.h"

void kernel_main() {
    terminal_init();
    terminal_putstring("Optimizing fish...\n");

    terminal_putstring("Done. MishaOS loaded.");
}