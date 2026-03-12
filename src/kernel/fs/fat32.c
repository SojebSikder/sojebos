#include "fat32.h"
#include <stdint.h>

extern void disk_read(uint32_t sector, uint8_t *buffer);

static FAT32 fs;

static uint32_t cluster_to_sector(uint32_t cluster)
{
    return ((cluster - 2) * fs.sectors_per_cluster)
            + fs.first_data_sector;
}

static uint32_t fat32_next_cluster(uint32_t cluster)
{
    uint8_t sector_buffer[512];
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs.reserved_sector_count + (fat_offset / 512);
    uint32_t entry_offset = fat_offset % 512;

    disk_read(fat_sector, sector_buffer);

    // Read the 32-bit value at the specific offset in the sector
    uint32_t next = *(uint32_t *)&sector_buffer[entry_offset];

    // FAT32 only uses the lower 28 bits
    return next & 0x0FFFFFFF;
}

void fat32_init()
{
    uint8_t boot[512];
    disk_read(0, boot);

    // Use dereferencing to get the actual values
    fs.bytes_per_sector = *(uint16_t *)(boot + 11);
    fs.sectors_per_cluster = *(uint8_t *)(boot + 13);
    fs.reserved_sector_count = *(uint16_t *)(boot + 14);
    fs.num_fats = *(uint8_t *)(boot + 16);
    fs.fat_size = *(uint32_t *)(boot + 36);
    fs.root_cluster = *(uint32_t *)(boot + 44);

    fs.first_data_sector = fs.reserved_sector_count + (fs.num_fats * fs.fat_size);
}

// direcotry listing
extern void console_print(char *);

static void print_name(char *name)
{
    char out[12];

    for (int i = 0; i < 11; i++)
        out[i] = name[i];

    out[11] = 0;

    console_print(out);
    console_print("\n");
}

void fat32_list_root()
{
    uint8_t buffer[512];

    uint32_t cluster = fs.root_cluster;

    while (cluster < 0x0FFFFFF8)
    {
        uint32_t sector = cluster_to_sector(cluster);

        disk_read(sector, buffer);

        fat32_dir_entry *entry =
            (fat32_dir_entry*)buffer;

        for (int i = 0; i < 16; i++)
        {
            if (entry[i].name[0] == 0x00)
                return;

            if (!(entry[i].attr & 0x0F))
                print_name(entry[i].name);
        }

        cluster = fat32_next_cluster(cluster);
    }
}
