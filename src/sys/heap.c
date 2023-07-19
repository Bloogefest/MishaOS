#include "heap.h"

#include <cpu/paging.h>

extern pfa_t pfa;
extern page_directory_t page_directory;

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

    int split_seg_length = seg->length - length - sizeof(heap_seg_hdr_t);
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

void heap_init(void* heap_address, size_t page_count) {
    void* virtual_address = heap_address;
    for (size_t i = 0; i < page_count; i++) {
        pde_map_memory(&page_directory, &pfa, virtual_address, pfa_request_page(&pfa));
        virtual_address = (void*) ((size_t) virtual_address + 0x1000);
    }

    size_t heap_length = page_count * 0x1000;
    heap_start = heap_address;
    heap_end = (void*) ((size_t) heap_start + heap_length);
    heap_seg_hdr_t* start_seg = (heap_seg_hdr_t*) heap_address;
    start_seg->length = heap_length - sizeof(heap_seg_hdr_t);
    start_seg->next = 0;
    start_seg->last = 0;
    start_seg->free = 1;
    last_header = start_seg;
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

void* malloc(size_t size) {
    if (size % 0x10) {
        size -= (size % 0x10);
        size += 0x10;
    }

    if (size == 0) {
        return 0;
    }

    heap_seg_hdr_t* current_seg = (heap_seg_hdr_t*) heap_start;
    while (1) {
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

        if (current_seg->next) {
            current_seg = current_seg->next;
        } else {
            break;
        }
    }

    heap_expand(size);
    return malloc(size);
}

void free(void* address) {
    heap_seg_hdr_t* segment = (heap_seg_hdr_t*) address - 1;
    segment->free = 1;
    heap_combine_forward(segment);
    heap_combine_backward(segment);
}
