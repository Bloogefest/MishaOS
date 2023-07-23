#include "mount.h"

#include <lib/string.h>
#include <sys/process.h>

static vfs_filesystem_t* filesystem;
static vfs_entry_t* root;

void mount_root(vfs_filesystem_t* root_filesystem, vfs_entry_t* new_root) {
    filesystem = root_filesystem;
    root = new_root;
}

vfs_filesystem_t* get_root_fs() {
    return filesystem;
}

vfs_entry_t* get_root_dir() {
    return root;
}

vfs_entry_t* open(const char* path) {
    vfs_entry_t* parent;
    if (*path == '/') {
        parent = root;
        ++path;
    } else {
        if (!current_process) {
            return 0;
        }

        parent = current_process->working_dir_entry;
    }

    char token[257];
    while (1) {
        char* token_end = strchr(path, '/');
        if (token_end == 0) {
            return vfs_find_entry_in(filesystem, parent, path);
        }

        memcpy(token, path, token_end - path);
        token[token_end - path] = 0;

        vfs_entry_t* next = vfs_find_entry_in(filesystem, parent, token);
        if (!next) {
            return 0;
        }

        parent = next;
        path = token_end + 1;
    }
}