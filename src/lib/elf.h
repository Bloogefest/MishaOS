#pragma once

#include <stdint.h>

#define ELFMAG0 0x7F
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define EI_NIDENT 16

typedef uint32_t elf32_word_t;
typedef uint32_t elf32_addr_t;
typedef uint32_t elf32_off_t;
typedef uint32_t elf32_sword_t;
typedef uint16_t elf32_half_t;

typedef struct elf32_header_s {
    uint8_t e_ident[EI_NIDENT];
    elf32_half_t e_type;
    elf32_half_t e_machine;
    elf32_word_t e_version;
    elf32_addr_t e_entry;
    elf32_off_t e_phoff;
    elf32_off_t e_shoff;
    elf32_word_t e_flags;
    elf32_half_t e_ehsize;
    elf32_half_t e_phentsize;
    elf32_half_t e_phnum;
    elf32_half_t e_shentsize;
    elf32_half_t e_shnum;
    elf32_half_t e_shstrndx;
} elf32_header_t;

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 2
#define ET_CORE 4
#define ET_LOPROC 0xFF0
#define ET_HIPROC 0xFFF

#define EM_NONE 0
#define EM_386 3

#define EV_NONE 0
#define EV_CURRENT 1

typedef struct elf32_phdr_s {
    elf32_word_t p_type;
    elf32_off_t p_offset;
    elf32_addr_t p_vaddr;
    elf32_addr_t p_paddr;
    elf32_word_t p_filesz;
    elf32_word_t p_memsz;
    elf32_word_t p_flags;
    elf32_word_t p_align;
} elf32_phdr_t;

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7FFFFFFF

typedef struct elf32_shdr_s {
    elf32_word_t sh_name;
    elf32_word_t sh_type;
    elf32_word_t sh_flags;
    elf32_word_t sh_addr;
    elf32_word_t sh_offset;
    elf32_word_t sh_size;
    elf32_word_t sh_link;
    elf32_word_t sh_info;
    elf32_word_t sh_addralign;
    elf32_word_t sh_entsize;
} elf32_shdr_t;

typedef struct elf32_auxv_s {
    uint32_t id;
    uint32_t ptr;
} elf32_auxv_t;

typedef struct elf32_sym_s {
    elf32_word_t st_name;
    elf32_addr_t st_value;
    elf32_word_t st_size;
    uint8_t st_info;
    uint8_t st_other;
    elf32_half_t st_shndx;
} elf32_sym_t;

typedef struct elf32_rel_s {
    elf32_addr_t r_offset;
    elf32_word_t r_info;
} elf32_rel_t;

typedef struct elf32_dyn_s {
    elf32_sword_t d_tag;
    union {
        elf32_word_t d_val;
        elf32_addr_t d_ptr;
        elf32_off_t d_off;
    } d_un;
} elf32_dyn_t;

#define SHT_NONE 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_NOBITS 8
#define SHT_REL 9

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((uint8_t) (i))
#define ELF32_R_INFO(s, t) (((s) << 8) + (uint8_t) (t))

#define ELF32_ST_BIND(i) ((i) >> 4)
#define ELF32_ST_TYPE(i) ((i) & 0xF)
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0xF))

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_NUM 3

#define STB_LOPROC 13
#define STB_HIPROC 15

#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_COMMON 5
#define STT_TLS 6
#define STT_NUM 7

#define STT_LOPROC 13
#define STT_HIPROC 15