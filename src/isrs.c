#include "isrs.h"
#include "terminal.h"

__attribute__((interrupt))
void interrupt_handler(struct interrupt_frame* frame) {
    terminal_putstring("Interrupt\n");
}