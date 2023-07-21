#pragma once

#include <stdint.h>
#include <multiboot.h>
#include <kernel.h>

typedef struct pfa_s {
    uint32_t size;
    uint8_t* buffer;
} pfa_t;

void pfa_read_memory_map(pfa_t* pfa, struct multiboot* multiboot, kernel_meminfo_t* meminfo, uint32_t initrd_start, uint32_t initrd_end);
void pfa_free_page(pfa_t* pfa, void* address);
void pfa_lock_page(pfa_t* pfa, void* address);
void pfa_free_pages(pfa_t* pfa, void* address, uint32_t count);
void pfa_lock_pages(pfa_t* pfa, void* address, uint32_t count);
void* pfa_request_page(pfa_t* pfa);
uint32_t pfa_free_memory();
uint32_t pfa_used_memory();
uint32_t pfa_reserved_memory();

typedef struct page_s {
    uint32_t present : 1;
    uint32_t read_write : 1;
    uint32_t user_supervisor : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t unused : 7;
    uint32_t address : 20;
} __attribute__((packed)) page_t;

typedef struct page_table_s {
    page_t entries[1024];
} __attribute__((packed)) page_table_t;

typedef struct page_directory_s {
    page_table_t* tables[1024];
    uint32_t physical_tables[1024];
    uint32_t physical_address;
    void* unaligned_ptr;
} __attribute__((packed)) page_directory_t;

extern page_directory_t* current_page_directory;

void pde_init(page_directory_t* page_directory);
page_directory_t* pde_alloc();
page_directory_t* pde_clone(page_directory_t* page_directory, pfa_t* pfa);
void pde_free(page_directory_t* page_directory, pfa_t* pfa);
page_t* pde_request_page(page_directory_t* page_directory, pfa_t* pfa, void* virtual_mem);
void pde_map_memory(page_directory_t* page_directory, pfa_t* pfa, void* virtual_mem, void* physical_mem);
void pde_map_user_memory(page_directory_t* page_directory, pfa_t* pfa, void* virtual_mem, void* physical_mem);
void* pde_get_phys_addr(page_directory_t* page_directory, void* virtual_addr);
void enable_paging(page_directory_t* page_directory);