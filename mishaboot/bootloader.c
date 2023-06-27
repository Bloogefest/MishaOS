__asm__(".code16gcc");

#include "iso9660.h"
#include "bootloader.h"
#include "memorymap.h"
#include "multiboot.h"
#include "util.h"
#include "elf.h"

#define DATA_LOAD_BASE 0x4000000

#define BIOS_READ_DISK 1
#define BIOS_QUERY_MODE 2
#define BIOS_SET_MODE 3

__attribute__((noinline)) __attribute__((regparm(3)))
void real_printchar(char ch) {
    __asm__ __volatile__("mov $0x0E, %%ah\n"
                         "mov %0, %%al\n"
                         "int $0x10" : : "a"(ch));
}

__attribute__((noinline)) __attribute__((regparm(3)))
void real_print(const char* str) {
    while (*str) {
        real_printchar(*str++);
    }
}

__attribute__((noinline)) __attribute__((regparm(3)))
void real_println(const char* str) {
    real_print(str);
    real_print("\r\n");
}

__attribute__((noinline)) __attribute__((regparm(3)))
void real_panic(const char* msg) {
    real_println("!!! MISHA BOOTLOADER PANIC !!!");
    real_println(msg);
    real_println("!!! CANNOT OPTIMIZE FISH !!!");

    while (1) {
        asm("hlt");
    }
}

__asm__(".code32");

void protected_printchar(char ch) {
    static int x = 0;
    static int y = 0;

    if (ch == '\n') {
        ++y;
        return;
    } else if (ch == '\r') {
        x = 0;
        return;
    }

    *((uint16_t*) 0xB8000 + y * 80 + x) = 0x0F00 | ch;
    ++x;
}

void protected_print(const char* str) {
    while (*str) {
        protected_printchar(*str++);
    }
}

void protected_println(const char* str) {
    protected_print(str);
    protected_print("\r\n");
}

void protected_panic(const char* msg) {
    protected_println("!!! MISHA BOOTLOADER PANIC !!!");
    protected_println(msg);
    protected_println("!!! CANNOT OPTIMIZE FISH !!!");

    while (1) {
        asm("hlt");
    }
}

void bios_call(char* target, uint32_t sector) {
    dap_num_sectors = 2048 / drive_params_bps;
    dap_buf_offset = (uint32_t) disk_space & 0xFFFF;
    dap_buf_segment = (uint32_t) disk_space >> 16;
    dap_lba_lower = sector * dap_num_sectors;
    dap_lba_upper = 0;
    do_bios_call(BIOS_READ_DISK, 0);
    memcpy(target, disk_space, 2048);
}

void clear_screen() {
    __asm__("mov $0, %al\n"
            "movl $3840, %ecx\n"
            "movl $0xb8000, %edi\n"
            "rep stosb");
}

extern uint32_t vbe_cont_info_mode_off;
extern uint16_t vbe_info_pitch;
extern uint16_t vbe_info_width;
extern uint16_t vbe_info_height;
extern uint8_t vbe_info_bpp;
extern vbe_info_t vbe_info;

void bios_set_video(int mode) {
    do_bios_call(BIOS_QUERY_MODE, mode);
    do_bios_call(BIOS_SET_MODE, mode);
}

void bios_video_mode() {
    int best_match = 0;
    int match_score = 0;

#define match(w, h, s) if (match_score < s && vbe_info_width == w && vbe_info_height == h) { best_match = *i; match_score = s; }

    uint32_t vbe_addr = ((vbe_cont_info_mode_off & 0xFFFF0000) >> 12) + (vbe_cont_info_mode_off & 0xFFFF);

    for (uint16_t* i = (uint16_t*) vbe_addr; *i != 0xFFFF; i++) {
        do_bios_call(BIOS_QUERY_MODE, *i);

        if (!(vbe_info.attributes & (1 << 7))) {
            continue;
        }

        if (vbe_info_bpp < 24) {
            continue;
        }

        if (vbe_info_bpp == 32) {
            if (match_score < 9) {
                best_match = *i;
                match_score = 9;
            }

            match(1024, 768, 10);
//            match(1280, 720, 50);
//            match(1280, 800, 60);
//            match(1440, 900, 75);
//            match(1920, 1080, 100);
        } else if (vbe_info_bpp == 24) {
            if (!match_score) {
                best_match = *i;
                match_score = 1;
            }

            match(1024, 768, 3);
//            match(1280, 720, 4);
//            match(1280, 800, 5);
//            match(1440, 900, 6);
//            match(1920, 1080, 7);
        }
    }

    if (best_match) {
        bios_set_video(best_match);
    } else {
        protected_panic("Video Mode not found.");
    }
}

void _boot(int smap_entry_count) {
    char buf[32];
    clear_screen();

    protected_println("=== MISHA BOOTLOADER [PROTECTED MODE] ===");

    utoa(drive_params_bps, buf, 10);
    protected_print("Disk BPS: ");
    protected_println(buf);

    protected_print("Looking for filesystem...");

    iso9660_volume_desc_t* root = 0;
    for (int i = 0x10; i < 0x15; ++i) {
        char* data = (char*) (DATA_LOAD_BASE + ISO_SECTOR_SIZE * i);
        bios_call(data, i);
        iso9660_volume_desc_t* volume = (void*) data;

        if (volume->type == 1) {
            protected_println(" OK.");
            root = volume;
            break;
        } else {
            protected_print(".");
        }
    }

    if (!root) {
        protected_println(" FAIL.");
        protected_panic("ISO9660 filesystem not found.");
        return;
    }

    iso9660_dir_entry_t* root_entry = (iso9660_dir_entry_t*) &root->root;
    protected_print("Looking for kernel... ");

    iso9660_dir_entry_t* kernel_entry;
    if (!iso9660_find_entry((char*) DATA_LOAD_BASE, root_entry, "KERNEL.ELF", &kernel_entry)) {
        protected_println("FAIL.");
        protected_panic("Kernel not found.");
        return;
    }

    protected_println("OK.");

    char* kernel_start = (char*) (DATA_LOAD_BASE + kernel_entry->extent_start_lsb * ISO_SECTOR_SIZE);

    protected_print("Loading kernel... ");
    for (int i = 0, j = 0; i < kernel_entry->extent_length_lsb; j++) {
        bios_call(kernel_start + i, kernel_entry->extent_start_lsb + j);
        i += ISO_SECTOR_SIZE;
    }

    protected_println("OK.");

    protected_print("Looking for initrd... ");

    iso9660_dir_entry_t* initrd_entry;
    if (!iso9660_find_entry((char*) DATA_LOAD_BASE, root_entry, "INITRD.IMG", &initrd_entry)) {
        protected_println("FAIL.");
        protected_panic("Initrd not found.");
        return;
    }

    protected_println("OK.");

    char* initrd_start = (char*) (DATA_LOAD_BASE + initrd_entry->extent_start_lsb * ISO_SECTOR_SIZE);
    uint32_t initrd_size = initrd_entry->extent_length_lsb;

    protected_print("Loading initrd... ");
    for (int i = 0, j = 0; i < initrd_size; j++) {
        bios_call(initrd_start + i, initrd_entry->extent_start_lsb + j);
        i += ISO_SECTOR_SIZE;
    }

    protected_println("OK.");

    for (uintptr_t i = 0; i < 8192; i += 4) {
        uint32_t* check = (uint32_t*) (kernel_start + i);
        if (*check == 0x1BADB002) {
            if (check[1] & (1 << 16)) {
                protected_panic("Unsupported format.");
            }

            break;
        }
    }

    protected_println("Parsing kernel...");

    elf32_header_t* header = (elf32_header_t*) kernel_start;

    if (header->e_ident[0] != ELFMAG0 ||
        header->e_ident[1] != ELFMAG1 ||
        header->e_ident[2] != ELFMAG2 ||
        header->e_ident[3] != ELFMAG3) {
        protected_panic("Invalid ELF32 magic value.");
        return;
    }

    uintptr_t final_offset = 0;

    uintptr_t entry = (uintptr_t) header->e_entry;
    for (uintptr_t i = 0; i < (uint32_t) header->e_phentsize * header->e_phnum; i += header->e_phentsize) {
        elf32_phdr_t* phdr = (elf32_phdr_t*) (kernel_start + header->e_phoff + i);
        if (phdr->p_type == PT_LOAD) {
            memcpy((uint8_t*)(uintptr_t) phdr->p_vaddr, kernel_start + phdr->p_offset, phdr->p_filesz);
            uintptr_t r = phdr->p_filesz;
            while (r < phdr->p_memsz) {
                *(char*)(phdr->p_vaddr + r) = 0;
                ++r;
            }

            if (phdr->p_vaddr + r > final_offset) {
                final_offset = phdr->p_vaddr + r;
            }
        }
    }

    // TODO: Check multiboot header

    clear_screen();
    bios_video_mode();

    final_offset = (final_offset & ~0xFFF) + ((final_offset & 0xFFF) ? 0x1000 : 0);
    uintptr_t relocated_initrd = final_offset;
    memcpy((void*) final_offset, initrd_start, initrd_size);
    final_offset += initrd_size;
    final_offset = (final_offset & ~0xFFF) + ((final_offset & 0xFFF) ? 0x1000 : 0);

    struct multiboot* multiboot_header = (void*) final_offset;
    final_offset += sizeof(struct multiboot);
    memset(multiboot_header, 0, sizeof(struct multiboot));
    multiboot_header->flags = MULTIBOOT_FLAG_MODS | MULTIBOOT_FLAG_MMAP | MULTIBOOT_FLAG_VBE;

    multiboot_module_t* module = (multiboot_module_t*) final_offset;
    final_offset += sizeof(multiboot_module_t);
    memset(module, 0, sizeof(multiboot_module_t));
    module->mod_start = relocated_initrd;
    module->mod_end = relocated_initrd + initrd_size;

    multiboot_header->mods_count = 1;
    multiboot_header->mods_addr = (uintptr_t) module;

    smap_entry_t* smap_entries = (smap_entry_t*) 0x1000;

    vbe_info_t* vbe = (vbe_info_t*) final_offset;
    final_offset += sizeof(vbe_info_t);
    memcpy(vbe, &vbe_info, sizeof(vbe_info_t));
    multiboot_header->vbe_mode_info = (uintptr_t) vbe;

    multiboot_memory_map_t* mmap = (void*) final_offset;
    multiboot_header->mmap_addr = (uintptr_t) mmap;

    for (int i = 0; i < smap_entry_count; i++) {
        memset(mmap, 0, sizeof(multiboot_memory_map_t));
        mmap->size = sizeof(uint64_t) * 2 + sizeof(uintptr_t);
        mmap->addr = smap_entries[i].base1;
        mmap->len = smap_entries[i].length1;
        mmap->type = smap_entries[i].type;
        ++mmap;
    }

    multiboot_header->mmap_length = (uintptr_t) mmap - multiboot_header->mmap_addr;

    uint32_t data[3];
    data[0] = MULTIBOOT_EAX_MAGIC;
    data[1] = (uint32_t) multiboot_header;
    data[2] = entry;
    __asm__ __volatile__("mov %%cr0, %%eax\n"
                         "and $0x7FFEFFFF, %%eax\n"
                         "mov %%eax, %%cr0\n"
                         "mov %%cr4, %%eax\n"
                         "and $0xFFFFFFDF, %%eax\n"
                         "mov %%eax, %%cr4\n"
                         "mov %1, %%eax\n"
                         "mov %2, %%ebx\n"
                         "jmp *%0" : : "g"(data[2]), "g"(data[0]), "g"(data[1]) : "eax", "ebx");
}

__asm__(".code16gcc");

__attribute__((noinline)) __attribute__((regparm(3)))
void _start() {
    real_println("Hello, MISHA!");

    smap_entry_t* smap_entries = (smap_entry_t*) 0x1000;
    const int smap_size = 0x2000;

    int entries = detect_memory(smap_entries, smap_size / sizeof(smap_entry_t));
    if (entries == -1) {
        real_panic("Unable to detect memory map.");
    }

    real_println("Entering Protected Mode...");
    enter_protected_mode();

    __asm__(".code32");

    _boot(entries);
}