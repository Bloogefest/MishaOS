#pragma once

#include <stdint.h>

#define VFS_SIGNATURE 0xF3F5

#define VFS_TYPE_FILE 0
#define VFS_TYPE_DIRECTORY 1
#define VFS_TYPE_LINK 2

typedef struct vfs_header_s {
    uint16_t signature;
    uint8_t version;
    char label[16];
    uint32_t size;
    uint32_t root_entry;
} __attribute__((packed)) vfs_header_t;

typedef struct vfs_entry_s {
    uint8_t zero;
    char name[256];
    uint32_t offset;
    uint8_t type;
    uint8_t reserved;
    uint32_t next_entry;
    uint32_t target_entry;
    uint32_t size;
} __attribute__((packed)) vfs_entry_t;

typedef struct vfs_filesystem_s {
    vfs_header_t header;
    uint8_t* data;
} vfs_filesystem_t;

vfs_filesystem_t* vfs_read_filesystem(vfs_filesystem_t* fs, uint8_t* data);
vfs_entry_t* vfs_find_entry_in(vfs_filesystem_t* fs, vfs_entry_t* entry, const char* name);
vfs_entry_t* vfs_find_entry(vfs_filesystem_t* fs, const char* name);
vfs_entry_t* vfs_follow_link(vfs_filesystem_t* fs, vfs_entry_t* entry);
vfs_entry_t* vfs_follow_links(vfs_filesystem_t* fs, vfs_entry_t* entry);
void* vfs_file_content(vfs_filesystem_t* fs, vfs_entry_t* entry, uint8_t follow_links);

