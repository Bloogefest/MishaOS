#include "heap.h"

#include <cpu/paging.h>
#include <sys/lock.h>

extern pfa_t pfa;
extern page_directory_t page_directory;

static volatile uint8_t heap_lock;

typedef struct heap_seg_hdr_s {
    size_t length;
    struct heap_seg_hdr_s* next;
    struct heap_seg_hdr_s* last;
    uint8_t free;
} heap_seg_hdr_t;

static void* heap_start;
static void* heap_end;
static heap_seg_hdr_t* last_header;

void heap_combine_forward(heap_seg_hdr_t* seg) {
    if (seg->next == 0) {
        return;
    }

    if (!seg->next->free) {
        return;
    }

    if (seg->next == last_header) {
        last_header = seg;
    }

    if (seg->next->next != 0) {
        seg->next->next->last = seg;
    }

    seg->length += seg->next->length + sizeof(heap_seg_hdr_t);
}

void heap_combine_backward(heap_seg_hdr_t* seg) {
    if (seg->last != 0 && seg->last->free) {
        heap_combine_forward(seg->last);
    }
}

heap_seg_hdr_t* heap_combine_split(heap_seg_hdr_t* seg, size_t length) {
    if (length < 0x10) {
        return 0;
    }

    size_t split_seg_length = seg->length - length - sizeof(heap_seg_hdr_t);
    if (split_seg_length < 0x10) {
        return 0;
    }

    heap_seg_hdr_t* new_header = (heap_seg_hdr_t*) ((size_t) seg + length + sizeof(heap_seg_hdr_t));
    if (seg->next) {
        seg->next->last = new_header;
    }
    new_header->next = seg->next;
    seg->next = new_header;
    new_header->last = seg;
    new_header->length = split_seg_length;
    new_header->free = seg->free;
    seg->length = length;

    if (last_header == seg) {
        last_header = new_header;
    }

    return new_header;
}

void heap_init(size_t page_count) {
    void* virtual_address = (void*) HEAP_START;
    for (size_t i = 0; i < page_count; i++) {
        pde_map_memory(&page_directory, &pfa, virtual_address, pfa_request_page(&pfa));
        virtual_address = (void*) ((size_t) virtual_address + 0x1000);
    }

    size_t heap_length = page_count * 0x1000;
    heap_start = (void*) HEAP_START;
    heap_end = (void*) ((size_t) heap_start + heap_length);
    heap_seg_hdr_t* start_seg = (heap_seg_hdr_t*)(void*) HEAP_START;
    start_seg->length = heap_length - sizeof(heap_seg_hdr_t);
    start_seg->next = 0;
    start_seg->last = 0;
    start_seg->free = 1;
    last_header = start_seg;
}

void heap_map_user_segment(void* address) {
    heap_seg_hdr_t* segment = (heap_seg_hdr_t*) address - 1;
    size_t length = segment->length;
    size_t pages = length / 0x1000 + (length & 0xFFF ? 1 : 0);
    for (size_t i = 0; i < pages; i++) {
        void* virtual_addr = (void*) ((uintptr_t) address + i * 0x1000);
        pde_map_user_memory(&page_directory, &pfa, virtual_addr, pde_get_phys_addr(&page_directory, virtual_addr));
    }
}

void heap_map_kernel_segment(void* address) {
    heap_seg_hdr_t* segment = (heap_seg_hdr_t*) address - 1;
    size_t length = segment->length;
    size_t pages = length / 0x1000 + (length & 0xFFF ? 1 : 0);
    for (size_t i = 0; i < pages; i++) {
        void* virtual_addr = (void*) ((uintptr_t) address + i * 0x1000);
        pde_map_memory(&page_directory, &pfa, virtual_addr, pde_get_phys_addr(&page_directory, virtual_addr));
    }
}

void heap_expand(size_t length) {
    if (length % 0x1000) {
        length -= length % 0x1000;
        length += 0x1000;
    }

    size_t page_count = length / 0x1000;
    heap_seg_hdr_t* new_segment = (heap_seg_hdr_t*) heap_end;
    for (size_t i = 0; i < page_count; i++) {
        pde_map_memory(&page_directory, &pfa, heap_end, pfa_request_page(&pfa));
        heap_end = (void*) ((size_t) heap_end + 0x1000);
    }

    new_segment->free = 1;
    new_segment->last = last_header;
    last_header->next = new_segment;
    last_header = new_segment;
    new_segment->next = 0;
    new_segment->length = length - sizeof(heap_seg_hdr_t);
    heap_combine_backward(new_segment);
}

void* kmalloc(size_t size) {
    if (size % 0x10) {
        size -= (size % 0x10);
        size += 0x10;
    }

    if (size == 0) {
        return 0;
    }

    heap_seg_hdr_t* current_seg = (heap_seg_hdr_t*) heap_start;
    while (current_seg) {
        if (current_seg->free) {
            if (current_seg->length > size) {
                heap_combine_split(current_seg, size);
                current_seg->free = 0;
                return (void*) (current_seg + 1);
            } else if (current_seg->length == size) {
                current_seg->free = 0;
                return (void*) (current_seg + 1);
            }
        }

        current_seg = current_seg->next;
    }

    heap_expand(size);
    return kmalloc(size);
}

#define alloc_begin()                                      \
    spin_lock(&heap_lock);                                 \
    page_directory_t* current_pd = current_page_directory; \
    enable_paging(&page_directory);

#define alloc_end()            \
    enable_paging(current_pd); \
    spin_unlock(&heap_lock);

void* malloc(size_t size) {
    alloc_begin();
    void* ptr = kmalloc(size);
    alloc_end();
    return ptr;
}

void kfree(void* address) {
    heap_map_kernel_segment(address);
    heap_seg_hdr_t* segment = (heap_seg_hdr_t*) address - 1;
    segment->free = 1;
    heap_combine_forward(segment);
    heap_combine_backward(segment);
}

void free(void* address) {
    alloc_begin();
    kfree(address);
    alloc_end();
    spin_unlock(&heap_lock);
}