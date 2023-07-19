#include "paging.h"

#include <lib/string.h>

uint32_t free_memory;
uint32_t reserved_memory;
uint32_t used_memory;
uint8_t initialized = 0;

static uint32_t page_index = 0;

static void pfa_reserve_page(pfa_t* pfa, void* address);
static void pfa_reserve_pages(pfa_t* pfa, void* address, uint32_t count);

void pfa_read_memory_map(pfa_t* pfa, struct multiboot* multiboot, kernel_meminfo_t* meminfo, uint32_t initrd_start, uint32_t initrd_end) {
    if (initialized) {
        return;
    } else {
        initialized = 1;
    }

    uint32_t largest_free_memory = 0;
    uint32_t largest_free_length = 0;

    multiboot_memory_map_t* entry = (multiboot_memory_map_t*) multiboot->mmap_addr;
    while ((uint32_t) entry < multiboot->mmap_addr + multiboot->mmap_length) {
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (entry->addr < 0x100000) {
                goto next;
            }

            if (entry->addr > UINT32_MAX) {
                goto next;
            }

            if (entry->len > UINT32_MAX) {
                entry->len = UINT32_MAX;
            }

            uint32_t address = (uint32_t) entry->addr;
            uint32_t length = (uint32_t) entry->len;

            if (address >= meminfo->kernel_physical_start && address + length >= meminfo->kernel_physical_end) {
                uint32_t end = address + length;
                address = initrd_end;
                length = end - address;
            } else if (address <= meminfo->kernel_physical_start && address + length >= meminfo->kernel_physical_start) {
                length = meminfo->kernel_physical_end - address - 1;
            } else if (address >= meminfo->kernel_physical_start && address + length <= meminfo->kernel_physical_end) {
                goto next;
            }

            if (address >= initrd_start && address + length >= initrd_end) {
                uint32_t end = address + length;
                address = initrd_end;
                length = end - address;
            } else if (address <= initrd_start && address + length >= initrd_start) {
                length = initrd_end - address - 1;
            } else if (address >= initrd_start && address + length <= initrd_end) {
                goto next;
            }

            if (length > largest_free_length) {
                largest_free_memory = address;
                largest_free_length = length;
            }
        }

        next:
        entry = (multiboot_memory_map_t*) (((uint32_t) entry) + entry->size + sizeof(entry->size));
    }

    free_memory = largest_free_length;
    uint64_t bitmap_size = free_memory / 0x1000 / 8 + 1;

    pfa->size = bitmap_size;
    pfa->buffer = (void*) largest_free_memory;
    memset(pfa->buffer, 0, bitmap_size);
    pfa_lock_pages(pfa, (void*) meminfo->kernel_physical_start,
                   (meminfo->kernel_physical_end - meminfo->kernel_physical_start) / 0x1000 + 1);
    pfa_lock_pages(pfa, (void*) initrd_start, (initrd_end - initrd_start) / 0x1000 + 1);

    entry = (multiboot_memory_map_t*) multiboot->mmap_addr;
    while ((uint32_t) entry < multiboot->mmap_addr + multiboot->mmap_length) {
        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
            pfa_reserve_pages(pfa, (void*) ((uint32_t) entry->addr), (uint32_t) (entry->len / 0x1000));
        }

        entry = (multiboot_memory_map_t*) (((uint32_t) entry) + entry->size + sizeof(entry->size));
    }

    free_memory = largest_free_length;
    pfa_lock_pages(pfa, pfa->buffer, pfa->size / 0x1000 + 1);
}

static inline uint8_t pfa_get_bit(pfa_t* pfa, uint32_t index) {
    if (index > pfa->size * 8) {
        return 0;
    }

    uint32_t byte = index / 8;
    uint8_t mask = (1 << 7) >> (index % 8);
    return !!(pfa->buffer[byte] & mask);
}

static inline uint8_t pfa_set_bit(pfa_t* pfa, uint32_t index, uint8_t value) {
    if (index > pfa->size * 8) {
        return 0;
    }

    uint32_t byte = index / 8;
    uint8_t mask = (1 << 7) >> (index % 8);
    if (value) {
        pfa->buffer[byte] |= mask;
    } else {
        pfa->buffer[byte] &= ~mask;
    }

    return 1;
}

void pfa_free_page(pfa_t* pfa, void* address) {
    uint32_t index = (uint32_t) address / 0x1000;
    if (!pfa_get_bit(pfa, index)) {
        return;
    }

    pfa_set_bit(pfa, index, 0);
    free_memory += 0x1000;
    used_memory -= 0x1000;
}

void pfa_lock_page(pfa_t* pfa, void* address) {
    uint32_t index = (uint32_t) address / 0x1000;
    if (pfa_get_bit(pfa, index)) {
        return;
    }

    pfa_set_bit(pfa, index, 1);
    free_memory -= 0x1000;
    used_memory += 0x1000;
}

void pfa_free_pages(pfa_t* pfa, void* address, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        pfa_free_page(pfa, (void*) ((uint32_t) address + i * 0x1000));
    }
}

void pfa_lock_pages(pfa_t* pfa, void* address, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        pfa_lock_page(pfa, (void*) ((uint32_t) address + i * 0x1000));
    }
}

static void pfa_reserve_page(pfa_t* pfa, void* address) {
    uint32_t index = (uint32_t) address / 0x1000;
    if (pfa_get_bit(pfa, index)) {
        return;
    }

    if (pfa_set_bit(pfa, index, 1)) {
        free_memory -= 0x1000;
        reserved_memory += 0x1000;
        if (page_index > index) {
            page_index = index;
        }
    }
}

static void pfa_reserve_pages(pfa_t* pfa, void* address, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        pfa_reserve_page(pfa, (void*) ((uint32_t) address + i * 0x1000));
    }
}

void* pfa_request_page(pfa_t* pfa) {
    for (; page_index < pfa->size * 8; page_index++) {
        if (pfa_get_bit(pfa, page_index)) {
            continue;
        }

        void* address = (void*) (page_index * 0x1000);
        pfa_lock_page(pfa, address);
        return address;
    }

    return 0; // TODO: Swap
}

uint32_t pfa_free_memory() {
    return free_memory;
}

uint32_t pfa_used_memory() {
    return used_memory;
}

uint32_t pfa_reserved_memory() {
    return reserved_memory;
}

void pde_init(page_directory_t* page_directory) {
    page_directory->physical_address = (uint32_t) &page_directory->physical_tables;
}

void pde_map_memory(page_directory_t* page_directory, pfa_t* pfa, void* virtual_mem, void* physical_mem) {
    uint32_t address = (uint32_t) virtual_mem / 0x1000;
    uint32_t table_idx = address / 1024;
    if (!page_directory->tables[table_idx]) {
        page_directory->tables[table_idx] = (page_table_t*) pfa_request_page(pfa);
        memset(page_directory->tables[table_idx], 0, 0x1000);
        page_directory->physical_tables[table_idx] = (uint32_t) page_directory->tables[table_idx] | 0x07;
    }

    page_t* page = &page_directory->tables[table_idx]->entries[address % 1024];
    page->present = 1;
    page->address = (uint32_t) physical_mem / 0x1000;
}

void* pde_get_phys_addr(page_directory_t* page_directory, void* virtual_addr) {
    uint32_t address = (uint32_t) virtual_addr / 0x1000;
    uint32_t pd_index = address / 1024;
    uint32_t pt_index = address % 1024;

    uint32_t phys_page = page_directory->tables[pd_index]->entries[pt_index].address;
    return (void*) (phys_page * 0x1000 + ((uint32_t) virtual_addr & 0xFFF));
}

void enable_paging(page_directory_t* page_directory) {
    asm volatile("mov %0, %%cr3" : : "r"(page_directory->physical_address));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}