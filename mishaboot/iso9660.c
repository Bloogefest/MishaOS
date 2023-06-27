#include "iso9660.h"

#include "bootloader.h"
#include "util.h"

void protected_println(const char* s);

int iso9660_find_entry(char* base, iso9660_dir_entry_t* parent, const char* name, iso9660_dir_entry_t** out) {
    char* dir_entries = base + parent->extent_start_lsb * ISO_SECTOR_SIZE;
    bios_call(dir_entries, parent->extent_start_lsb);
    long offset = 0;
    while (1) {
        iso9660_dir_entry_t* dir = (iso9660_dir_entry_t*) (dir_entries + offset);
        if (dir->length == 0) {
            if (offset < parent->extent_length_lsb) {
                offset += 1;
                goto next;
            }

            return 0;
        }

        if (!(dir->flags & FLAG_HIDDEN)) {
            char filename[dir->name_len + 1];
            memcpy(filename, dir->name, dir->name_len);
            filename[dir->name_len] = 0;

            char* s = strchr(filename, ';');
            if (s) {
                *s = 0;
            }

            if (strcmp(filename, name) == 0) {
                *out = dir;
                return 1;
            }
        }

        offset += dir->length;

        next:
        if (offset > parent->extent_length_lsb) {
            return 0;
        }
    }
}