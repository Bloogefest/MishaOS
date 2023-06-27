__asm__(".code16gcc\n");

#include "memorymap.h"

__attribute__((noinline)) __attribute__((regparm(3)))
int detect_memory(smap_entry_t* buffer, int count) {
    uint32_t cont_id = 0;
    int entries = 0;
    int signature;
    int bytes;

    do {
        __asm__ __volatile__("int $0x15"
                            : "=a"(signature), "=c"(bytes), "=b"(cont_id)
                            : "a"(0xE820), "b"(cont_id), "c"(24), "d"(0x534D4150), "D"(buffer));
        if (signature != 0x534D4150) {
            return -1;
        }

        if (bytes > 20 && (buffer->acpi & 0x0001)) {
            continue;
        } else {
            ++buffer;
            ++entries;
        }
    } while (cont_id != 0 && entries < count);

    return entries;
}