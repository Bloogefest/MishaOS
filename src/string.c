#include "string.h"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }

    return len;
}

void* memset(void* ptr, int value, size_t size) {
    uint8_t* data = (uint8_t*) ptr;
    for (size_t i = 0; i < size; i++) {
        data[i] = value;
    }

    return ptr;
}