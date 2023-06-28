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
            entries = -1;
            break;
        }

        if (bytes > 20 && (buffer->acpi & 0x0001)) {
            continue;
        } else {
            ++buffer;
            ++entries;
        }
    } while (cont_id != 0 && entries < count);

    if (entries <= 0) {
        uint16_t ax;
        uint16_t bx;
        uint16_t cx;
        uint16_t dx;
        __asm__ __volatile__("int $0x15"
                            : "=a"(ax), "=b"(bx), "=c"(cx), "=d"(dx)
                            : "a"(0xE801));

        if (ax == 0 && bx == 0) {
            ax = cx;
            bx = dx;
        }

        uint8_t ah = ax >> 8;
        if (ah == 0x86 || ah == 0x80) {
            return -1;
        }

        uint32_t to16mb_bytes = (uint32_t) ax * 1024;
        uint32_t above16mb_bytes = (uint32_t) bx * 64 * 1024;

        entries = 3;

        buffer->base1 = 0; // TODO: Detect lower memory?
        buffer->base2 = 0;
        buffer->length1 = 0x100000;
        buffer->length2 = 0;
        buffer->type = 2;
        ++buffer;

        buffer->base1 = 0x100000;
        buffer->base2 = 0;
        buffer->length1 = to16mb_bytes;
        buffer->length2 = 0;
        buffer->type = 1;
        ++buffer;

        buffer->base1 = 0x100000 + to16mb_bytes;
        buffer->base2 = 0;
        buffer->length1 = 15 * 0x100000 - to16mb_bytes;
        buffer->length2 = 0;
        buffer->type = 2;
        if (buffer->length1) {
            ++buffer;
            ++entries;
        }

        buffer->base1 = 16 * 0x100000;
        buffer->base2 = 0;
        buffer->length1 = above16mb_bytes;
        buffer->length2 = 0;
        buffer->type = 1;
    }

    return entries;
}