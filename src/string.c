#include "string.h"
#include "terminal.h"

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

char* strstr(const char* a, const char* b) {
    char* current = (char*) a;
    while (1) {
        char* result = current++;
        for (size_t i = 0; b[i]; i++) {
            if (!result[i]) {
                return 0;
            } else if (result[i] != b[i]) {
                result = 0;
                break;
            }
        }

        if (result) {
            return result;
        }
    }
}

char* _strstr(const char* a, const char* b, size_t limit) {
    char* current = (char*) a;
    for (size_t j = 0; j <= limit - strlen(b); j++) {
        char* result = current++;
        for (size_t i = 0; b[i]; i++) {
            if (result[i] != b[i]) {
                result = 0;
                break;
            }
        }

        if (result) {
            return result;
        }
    }

    return 0;
}

int strcmp(const char* a, const char* b) {
    for (size_t i = 0;; i++) {
        if (a[i] < b[i]) {
            return -1;
        } else if (a[i] > b[i]) {
            return 1;
        } else if (a[i] == 0) {
            return 0;
        }
    }
}

int _strcmp(const char* a, const char* b, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (a[i] < b[i]) {
            return -1;
        } else if (a[i] > b[i]) {
            return 1;
        }
    }

    return 0;
}

void* memcpy(void* dst, void* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((uint8_t*) dst)[i] = ((uint8_t*) src)[i];
    }

    return dst;
}