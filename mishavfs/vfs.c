#include "vfs.h"

#include <string.h>

static inline vfs_entry_t* vfs_get_entry(vfs_filesystem_t* fs, uint32_t offset) {
    return (vfs_entry_t*) (fs->data + offset);
}

vfs_filesystem_t* vfs_read_filesystem(vfs_filesystem_t* fs, uint8_t* data) {
    if (!fs) {
        return 0;
    }

    fs->data = data;
    memcpy(&fs->header, data, sizeof(vfs_header_t));
    if (fs->header.signature != VFS_SIGNATURE) {
        return 0; // Invalid signature
    }

    if (fs->header.version != 1) {
        return 0; // Unsupported version
    }

    if (fs->header.size < sizeof(vfs_filesystem_t)) {
        return 0; // Invalid size
    }

    return fs;
}

vfs_entry_t* vfs_find_entry_in(vfs_filesystem_t* fs, vfs_entry_t* entry, const char* name) {
    entry = vfs_follow_links(fs, entry);

    if (entry->type != VFS_TYPE_DIRECTORY || !entry->target_entry) {
        return 0;
    }

    uint32_t current_entry = entry->target_entry;
    while (current_entry) {
        entry = vfs_get_entry(fs, current_entry);
        if (strcmp(name, entry->name) == 0) {
            return entry;
        }

        current_entry = entry->next_entry;
    }

    return 0;
}

vfs_entry_t* vfs_find_entry(vfs_filesystem_t* fs, const char* name) {
    if (!fs || !name) {
        return 0;
    }

    return vfs_find_entry_in(fs, vfs_get_entry(fs, fs->header.root_entry), name);
}

vfs_entry_t* vfs_follow_link(vfs_filesystem_t* fs, vfs_entry_t* entry) {
    if (entry->type == VFS_TYPE_LINK) {
        return vfs_get_entry(fs, entry->target_entry);
    } else {
        return entry;
    }
}

vfs_entry_t* vfs_follow_links(vfs_filesystem_t* fs, vfs_entry_t* entry) {
    while (entry->type == VFS_TYPE_LINK) {
        entry = vfs_get_entry(fs, entry->target_entry);
    }

    return entry;
}

void* vfs_file_content(vfs_filesystem_t* fs, vfs_entry_t* entry, uint8_t follow_links) {
    if (follow_links) {
        entry = vfs_follow_links(fs, entry);
    }

    return (void*) (fs->data + entry->offset + sizeof(vfs_entry_t));
}