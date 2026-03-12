#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

typedef struct {

    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;

    uint8_t num_fats;
    uint32_t fat_size;

    uint32_t root_cluster;

    uint32_t first_data_sector;

} FAT32;

typedef struct {

    char name[11];
    uint8_t attr;
    uint8_t nt_reserved;
    uint8_t create_time_tenth;

    uint16_t create_time;
    uint16_t create_date;

    uint16_t access_date;

    uint16_t first_cluster_high;

    uint16_t write_time;
    uint16_t write_date;

    uint16_t first_cluster_low;

    uint32_t file_size;

} __attribute__((packed)) fat32_dir_entry;

void fat32_init();
void fat32_list_root();
void fat32_cat(const char* filename);

#endif
