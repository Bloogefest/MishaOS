#include "util.h"

int toupper(int c) {
    if(c >= 'a' && c <= 'z') {
        return c - ('a' - 'A');
    }

    return c;
}

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

int memcmp(void* ptr1, void* ptr2, size_t size) {
    uint8_t* a = (uint8_t*) ptr1;
    uint8_t* b = (uint8_t*) ptr2;
    for (size_t i = 0; i < size; i++) {
        if (a[i] < b[i]) {
            return -1;
        } else if (a[i] > b[i]) {
            return 1;
        }
    }

    return 0;
}

char* _strchr(const char* str, int chr, uint32_t limit) {
    for (uint32_t i = 0; i < limit; i++) {
        if (str[i] == chr) {
            return (char*) (str + i);
        } else if (str[i] == 0) {
            return 0;
        }
    }

    return 0;
}

char* strchr(const char* str, int chr) {
    for (;; str++) {
        if (*str == chr) {
            return (char*) str;
        } else if (*str == 0) {
            return 0;
        }
    }
}

int strcmp(const char* str1, const char* str2) {
    const uint8_t* a = (const uint8_t*) str1;
    const uint8_t* b = (const uint8_t*) str2;
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

void* memcpy(void* dst, const void* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((uint8_t*) dst)[i] = ((const uint8_t*) src)[i];
    }

    return dst;
}

size_t memrev(char* buf, size_t n) {
    size_t i;
    for (i = 0; i < n / 2; ++i) {
        const char x = buf[i];
        buf[i] = buf[n - i - 1];
        buf[n - i - 1] = x;
    }

    return i;
}

size_t strrev(char* s) {
    return memrev(s, strlen(s));
}

char* itoa(int32_t target, char* buf, uint32_t radix) {
    if (radix < 2 || radix > 36) {
        return 0;
    }

    int sign = 0;
    if (target < 0) {
        sign = 1;
        target = -target;
    }

    char* low = buf;
    do {
        *buf++ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[target % radix];
    } while (target /= radix);

    if (sign) {
        *buf++ = '-';
    }

    *buf = '\0';
    strrev(low);
    return buf;
}

char* utoa(int32_t target, char* buf, uint32_t radix) {
    if (radix < 2 || radix > 36) {
        return 0;
    }

    char* low = buf;
    do {
        *buf++ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[target % radix];
    } while (target /= radix);

    *buf = '\0';
    strrev(low);
    return buf;
}

int32_t atoi(const char* buf, uint32_t radix) {
    int32_t res = 0;
    uint8_t sign = 0;
    if (radix < 2 || radix > 36) {
        return -4;
    }

    char* ptr = (char*) buf;
    char c;
    static const char* alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while ((c = *ptr++) != 0) {
        if (c == '-') {
            sign = 1;
            continue;
        }

        if (strchr("\t\n\v\f\r +_", c) != NULL) {
            continue;
        }

        char* x = strchr(alphabet, toupper(c));
        if (x == 0) {
            break;
        }

        size_t pos = x - alphabet;
        if (pos >= radix) {
            break;
        }

        res = res * radix + pos;
    }

    return sign ? -res : res;
}