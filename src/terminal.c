#include "terminal.h"
#include "string.h"

size_t terminal_row;
size_t terminal_column;

terminal_t terminal;

void terminal_init() {
    terminal.init();
}

void terminal_set_row(size_t row) {
    terminal_row = row;
}

void terminal_set_column(size_t column) {
    terminal_column = column;
}

size_t terminal_get_row() {
    return terminal_row;
}

size_t terminal_get_column() {
    return terminal_column;
}

void terminal_clear_terminal() {
    terminal.clear();
}

void terminal_putchar(char c) {
    terminal.putchar(c);
}

void terminal_put(const char* str, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(str[i]);
    }
}

void terminal_putstring(const char* str) {
    terminal_put(str, strlen(str));
}

size_t terminal_get_columns() {
    return terminal.columns;
}

size_t terminal_get_rows() {
    return terminal.rows;
}