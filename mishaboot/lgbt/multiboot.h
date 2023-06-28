#pragma once

#include <stdint.h>

#define MULTIBOOT_MAGIC 0x1BADB002
#define MULTIBOOT_EAX_MAGIC 0x2BADB002
#define MULTIBOOT_FLAG_MEM 0x001
#define MULTIBOOT_FLAG_DEVICE 0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS 0x008
#define MULTIBOOT_FLAG_AOUT 0x010
#define MULTIBOOT_FLAG_ELF 0x020
#define MULTIBOOT_FLAG_MMAP 0x040
#define MULTIBOOT_FLAG_DRIVE 0x080
#define MULTIBOOT_FLAG_CONFIG 0x100
#define MULTIBOOT_FLAG_LOADER 0x200
#define MULTIBOOT_FLAG_APM 0x400
#define MULTIBOOT_FLAG_VBE 0x800
#define MULTIBOOT_FLAG_FB 0x1000

struct multiboot {
    uintptr_t flags;
    uintptr_t mem_lower;
    uintptr_t mem_upper;
    uintptr_t boot_device;
    uintptr_t cmdline;
    uintptr_t mods_count;
    uintptr_t mods_addr;
    uintptr_t num;
    uintptr_t size;
    uintptr_t addr;
    uintptr_t shndx;
    uintptr_t mmap_length;
    uintptr_t mmap_addr;
    uintptr_t drives_length;
    uintptr_t drives_addr;
    uintptr_t config_table;
    uintptr_t boot_loader_name;
    uintptr_t apm_table;
    uintptr_t vbe_control_info;
    uintptr_t vbe_mode_info;
    uintptr_t vbe_mode;
    uintptr_t vbe_interface_seg;
    uintptr_t vbe_interface_off;
    uintptr_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
};

typedef struct vbe_info_s {
    uint16_t attributes;
    uint8_t win_a;
    uint8_t win_b;
    uint16_t granularity;
    uint16_t winsize;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t real_fct_ptr;
    uint16_t pitch;
    uint16_t x_res;
    uint16_t y_res;
    uint8_t w_char;
    uint8_t y_char;
    uint8_t planes;
    uint8_t bpp;
    uint8_t banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t image_pages;
    uint8_t reserved0;
    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t rsv_mask;
    uint8_t rsv_position;
    uint8_t directcolor_attributes;
    uint32_t physbase;
    uint32_t reserved1;
    uint16_t reserved2;
} vbe_info_t;

typedef struct multiboot_module_s {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t cmdline;
    uint32_t reserved;
} __attribute__((packed)) multiboot_module_t;

typedef struct multiboot_memory_map_s {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_memory_map_t;