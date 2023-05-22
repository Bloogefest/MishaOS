#include "stdlib.h"
#include <stddef.h>
#include <stdbool.h>

static void reverse(char* str, size_t length) {
    size_t start = 0;
    size_t end = length - 1;
    while (start < end) {
        char t = str[start];
        str[start++] = str[end];
        str[end--] = t;
    }
}

char* itoa(int n, char* buf, int radix) {
    int i = 0;
    bool neg = false;
    if (n == 0) {
        buf[i++] = '0';
        buf[i] = '\0';
        return buf;
    }

    if (n < 0 && radix == 10) {
        neg = true;
        n = -n;
    }

    while (n != 0) {
        int rem = n % radix;
        buf[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        n = n / radix;
    }

    if (neg) {
        buf[i++] = '-';
    }

    buf[i] = '\0';
    reverse(buf, i);
    return buf;
}