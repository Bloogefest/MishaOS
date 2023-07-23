#include "exec.h"

#include <lib/kprintf.h>
#include <lib/string.h>
#include <sys/kernel_mem.h>
#include <sys/process.h>
#include <sys/mount.h>
#include <sys/heap.h>
#include <misc/elf.h>

int exec(const char* path, int argc, const char** argv) {
    vfs_entry_t* file = open(path);
    if (!file) {
        return 0;
    }

    uint8_t* exec_data = vfs_file_content(get_root_fs(), file, 0);
    elf32_header_t* header = (elf32_header_t*) exec_data;

    free(current_process->name);
    current_process->name = strdup(path);

    if (header->e_ident[0] != ELFMAG0 ||
        header->e_ident[1] != ELFMAG1 ||
        header->e_ident[2] != ELFMAG2 ||
        header->e_ident[3] != ELFMAG3) {
        kprintf("exec: '%s' is not a valid executable file.\n", path);
        return -1;
    }

    uintptr_t entry = (uintptr_t) header->e_entry;
    uintptr_t final_offset = 0;
    for (uintptr_t p = 0; p < (uint32_t) header->e_phentsize * header->e_phnum; p += header->e_phentsize) {
        elf32_phdr_t* phdr = (elf32_phdr_t*) (exec_data + header->e_phoff + p);
        if (phdr->p_type == PT_LOAD) {
            uint32_t pages = phdr->p_memsz / 0x1000;
            if (phdr->p_memsz & 0xFFF) {
                ++pages;
            }

            for (uint32_t i = 0; i < pages; i++) {
                uint32_t offset = i * 4096;

                void* page = pfa_request_page(&pfa);
                memset(page, 0, 4096);
                pde_map_user_memory(current_page_directory, &pfa, (void*)(uintptr_t)(phdr->p_vaddr + offset), page);

                if (offset <= phdr->p_filesz) {
                    uint32_t length = 4096;
                    if (offset + length > phdr->p_filesz) {
                        length = phdr->p_filesz - offset;
                    }
                    memcpy(page, exec_data + phdr->p_offset + offset, length);
                }
            }

            if (phdr->p_vaddr < current_process->image.entry) {
                current_process->image.entry = phdr->p_vaddr;
            }

            if (phdr->p_vaddr + phdr->p_memsz > final_offset) {
                final_offset = phdr->p_vaddr + phdr->p_memsz;
            }
        }
    }

    current_process->image.size = final_offset - current_process->image.entry;

    for (uintptr_t stack_pointer = 0x10000000; stack_pointer < 0x10010000; stack_pointer += 0x1000) {
        pde_map_user_memory(current_page_directory, &pfa, (void*) stack_pointer, pfa_request_page(&pfa));
    }

    uintptr_t heap = (final_offset & ~0xFFF) + ((final_offset & 0xFFF) ? 0x1000 : 0);
    uintptr_t current_page = heap / 0x1000;

    pde_map_user_memory(current_page_directory, &pfa, (void*) heap, pfa_request_page(&pfa));

#define expand_heap()                                                                                               \
    while (current_page < heap / 0x1000) {                                                                          \
        ++current_page;                                                                                             \
        pde_map_user_memory(current_page_directory, &pfa, (void*) (current_page * 0x1000), pfa_request_page(&pfa)); \
    }

    elf32_auxv_t auxv = {0, 0};

    char** argv_ptr = (char**) heap;
    heap += sizeof(char*) * (argc + 2);
    expand_heap();
    char** env_ptr = (char**) heap;
    heap += sizeof(char*) * 1;
    expand_heap();
    void* auxv_ptr = (void*) heap;
    heap += sizeof(auxv);
    expand_heap();

    for (int i = -1; i < argc; i++) {
        const char* arg = i == -1 ? path : argv[i];
        uintptr_t target_ptr = heap;
        heap += strlen(arg) + 1;
        expand_heap();
        strcpy((void*) target_ptr, arg);
        argv_ptr[i + 1] = (char*) target_ptr;
    }

    argv_ptr[argc + 1] = 0;
    env_ptr[0] = 0;
    memcpy(auxv_ptr, &auxv, sizeof(auxv));

    current_process->image.heap = heap;
    current_process->image.heap_aligned = heap + (0x1000 - heap % 0x1000);
    current_process->image.user_stack = 0x10010000;
    current_process->image.start = entry;
    enter_userspace(entry, 0x10010000, argc + 1, (const char**) argv_ptr);
    return -1;
}

int system(const char* path, int argc, const char** argv) {
    int child = fork();
    if (child == 0) {
        exec(path, argc, argv);
        return -1;
    } else {
        switch_next();
        return -1;
    }
}