#include "paging.h"

#include <lib/string.h>
#include <lib/kprintf.h>
#include <sys/heap.h>

uint32_t free_memory;
uint32_t reserved_memory;
uint32_t used_memory;
uint8_t initialized = 0;

page_directory_t* current_page_directory = 0;

static uint32_t page_index = 0;

static void pfa_reserve_page(pfa_t* pfa, void* address);
static void pfa_reserve_pages(pfa_t* pfa, void* address, uint32_t count);

void pfa_read_memory_map(pfa_t* pfa, struct multiboot* multiboot, kernel_meminfo_t* meminfo, uint32_t initrd_start, uint32_t initrd_end) {
    if (initialized) {
        return;
    } else {
        initialized = 1;
    }

    pfa->size = UINT32_MAX / 0x1000 / 8 + 1;
    pfa->buffer = (void*) 0x1000000;
    memset(pfa->buffer, 0, pfa->size);

    uint64_t total_free_length = 0;

    multiboot_memory_map_t* entry = (multiboot_memory_map_t*) multiboot->mmap_addr;
    while ((uint32_t) entry < multiboot->mmap_addr + multiboot->mmap_length) {
        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
            if (entry->addr < UINT32_MAX) {
                pfa_reserve_pages(pfa, (void*) ((uint32_t) entry->addr), (uint32_t) (entry->len / 0x1000));
            }
        } else {
            total_free_length += entry->len;
        }

        entry = (multiboot_memory_map_t*) (((uint32_t) entry) + entry->size + sizeof(entry->size));
    }

    if (total_free_length > UINT32_MAX) {
        free_memory = UINT32_MAX;
    } else {
        free_memory = (uint32_t) total_free_length;
    }

    pfa_lock_pages(pfa, (void*) meminfo->kernel_physical_start,
                   (meminfo->kernel_physical_end - meminfo->kernel_physical_start) / 0x1000 + 1);
    pfa_lock_pages(pfa, (void*) initrd_start, (initrd_end - initrd_start) / 0x1000 + 1);
    pfa_lock_pages(pfa, pfa->buffer, pfa->size / 0x1000 + 1);
}

static inline uint8_t pfa_get_bit(pfa_t* pfa, uint32_t index) {
    if (index > pfa->size * 8) {
        return 0;
    }

    uint32_t byte = index / 8;
    uint8_t mask = (1 << 7) >> (index % 8);
    return (pfa->buffer[byte] & mask) != 0;
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

    kprintf("[Error] Out of memory.\n");
    return 0; // TODO: Swap
}

void* pfa_request_pages(pfa_t* pfa, uint32_t pages) {
    for (; pfa_get_bit(pfa, page_index); page_index++);
    uint32_t current_index = page_index;
    while (1) {
        uint8_t free = 1;
        for (uint32_t i = 0; i < pages; i++) {
            if (pfa_get_bit(pfa, (current_index + i) * 0x1000)) {
                free = 0;
                break;
            }
        }

        if (free) {
            void* address = (void*) (current_index * 0x1000);
            pfa_lock_pages(pfa, address, pages);
            return address;
        }

        current_index += pages;
    }
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

void pde_init(page_directory_t* page_directory, pfa_t* pfa) {
    void* ptr = &page_directory->physical_tables;
    if (current_page_directory) {
        ptr = pde_get_phys_addr(current_page_directory, ptr);
    }

    page_directory->physical_address = (uint32_t)(uintptr_t) ptr;
    for (uint32_t i = 0; i < 1024; i++) {
        if (page_directory->tables[i]) {
            page_directory->physical_tables[i] = (uint32_t) page_directory->tables[i] | 0x07;
        } else if (i >= HEAP_START_TABLE && i < HEAP_END_TABLE) {
            page_directory->tables[i] = pfa_request_page(pfa);
            page_directory->physical_tables[i] = (uint32_t) page_directory->tables[i] | 0x07;
        }
    }
}

page_directory_t* pde_alloc(pfa_t* pfa) {
    uintptr_t address = (uintptr_t) pfa_request_pages(pfa, 3);

    page_directory_t* page_directory = (page_directory_t*) address;
    memset(page_directory, 0, sizeof(page_directory_t));
    return page_directory;
}

page_directory_t* pde_clone(page_directory_t* page_directory, pfa_t* pfa) {
    page_directory_t* clone = pde_alloc(pfa);

    for (uint32_t i = 0; i < 1024; i++) {
        if (i >= HEAP_START_TABLE && i < HEAP_END_TABLE) {
            clone->tables[i] = page_directory->tables[i];
        } else if (page_directory->tables[i] && (uintptr_t) page_directory->tables[i] != 0xFFFFFFFF) {
            clone->tables[i] = pfa_request_page(pfa);
            memcpy(clone->tables[i], page_directory->tables[i], 4096);
        }
    }

    pde_init(clone, pfa);
    return clone;
}

void pde_free(page_directory_t* page_directory, pfa_t* pfa) {
    for (uint32_t i = 0; i < 1024; i++) {
        if (i >= HEAP_START_TABLE && i < HEAP_END_TABLE) {
            continue;
        }

        if (page_directory->tables[i]) {
            pfa_free_page(pfa, page_directory->tables[i]);
        }
    }

    pfa_free_pages(pfa, page_directory, 3);
}

page_t* pde_request_page(page_directory_t* page_directory, pfa_t* pfa, void* virtual_mem) {
    uint32_t address = (uint32_t) virtual_mem / 0x1000;
    uint32_t table_idx = address / 1024;
    if (!page_directory->tables[table_idx]) {
        page_directory->tables[table_idx] = (page_table_t*) pfa_request_page(pfa);
        memset(page_directory->tables[table_idx], 0, 0x1000);
        page_directory->physical_tables[table_idx] = (uint32_t) page_directory->tables[table_idx] | 0x07;
    }

    return &page_directory->tables[table_idx]->entries[address % 1024];
}

void pde_map_memory(page_directory_t* page_directory, pfa_t* pfa, void* virtual_mem, void* physical_mem) {
    page_t* page = pde_request_page(page_directory, pfa, virtual_mem);
    page->present = 1;
    page->address = (uint32_t) physical_mem / 0x1000;
}

void pde_map_user_memory(page_directory_t* page_directory, pfa_t* pfa, void* virtual_mem, void* physical_mem) {
    page_t* page = pde_request_page(page_directory, pfa, virtual_mem);
    page->present = 1;
    page->read_write = 1;
    page->user_supervisor = 1;
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
    current_page_directory = page_directory;
    asm volatile("mov %0, %%cr3" : : "r"(page_directory->physical_address));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}