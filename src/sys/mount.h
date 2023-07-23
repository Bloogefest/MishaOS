#pragma once

#include <vfs.h>

void mount_root(vfs_filesystem_t* root_filesystem, vfs_entry_t* new_root);
vfs_filesystem_t* get_root_fs();
vfs_entry_t* get_root_dir();
vfs_entry_t* open(const char* path);