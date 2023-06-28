#pragma once

#include <stdint.h>

typedef struct iso9660_datetime_s {
    char year[4];
    char month[2];
    char day[2];
    char hour[2];
    char minute[2];
    char second[2];
    char hundredths[2];
    int8_t timezone;
} __attribute__((packed)) iso9660_datetime_t;

typedef struct iso9660_rec_date_s {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    int8_t timezone;
} __attribute__((packed)) iso9660_rec_date_t;

typedef struct iso9660_dir_entry_s {
    uint8_t length;
    uint8_t ext_length;
    uint32_t extent_start_lsb;
    uint32_t extent_start_msb;
    uint32_t extent_length_lsb;
    uint32_t extent_length_msb;
    iso9660_rec_date_t record_date;
    uint8_t flags;
    uint8_t interleave_units;
    uint8_t interleave_gap;
    uint16_t volume_seq_lsb;
    uint16_t volume_seq_msb;
    uint8_t name_len;
    char name[];
} __attribute__((packed)) iso9660_dir_entry_t;

typedef struct iso9660_volume_desc_s {
    uint8_t type;
    char id[5];
    uint8_t version;
    uint8_t unused0;
    char system_id[32];
    char volume_id[32];
    uint8_t unused1[8];
    uint32_t volume_space_lsb;
    uint32_t volume_space_msb;
    uint8_t unused2[32];
    uint16_t volume_set_lsb;
    uint16_t volume_set_msb;
    uint16_t volume_seq_lsb;
    uint16_t volume_seq_msb;
    uint16_t logical_block_size_lsb;
    uint16_t logical_block_size_msb;
    uint32_t path_table_size_lsb;
    uint32_t path_table_size_msb;
    uint32_t path_table_lsb;
    uint32_t opt_path_table_lsb;
    uint32_t path_table_msb;
    uint32_t opt_path_table_msb;
    char root[34];
    char volume_set_id[128];
    char volume_publisher[128];
    char data_preparer[128];
    char application_id[128];
    char copyright_file[38];
    char abstract_file[36];
    char bibliographic_file[37];
    iso9660_datetime_t creation;
    iso9660_datetime_t modification;
    iso9660_datetime_t expiration;
    iso9660_datetime_t effective;
    uint8_t file_structure_version;
    uint8_t unused3;
    char application_use[];
} __attribute__((packed)) iso9660_volume_desc_t;

#define ISO_SECTOR_SIZE 2048

#define FLAG_HIDDEN 0x01
#define FLAG_DIRECTORY 0x02
#define FLAG_ASSOCIATED 0x04
#define FLAG_EXTENDED 0x08
#define FLAG_PERMISSIONS 0x10
#define FLAG_CONTINUES 0x80

int iso9660_find_entry(char* base, iso9660_dir_entry_t* parent, const char* name, iso9660_dir_entry_t** out);