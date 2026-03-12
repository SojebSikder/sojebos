#include "fat32.h"
#include "../../drivers/console.h"
#include <stdint.h>

extern void disk_read(uint32_t sector, uint8_t *buffer);

static FAT32 fs;

static uint32_t cluster_to_sector(uint32_t cluster) {
  return ((cluster - 2) * fs.sectors_per_cluster) + fs.first_data_sector;
}

static uint32_t fat32_next_cluster(uint32_t cluster) {
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

void fat32_init() {
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

// directory listing
static void print_name(char *name) {
  char out[13]; // 8 (name) + 1 (.) + 3 (ext) + 1 (null)
  int out_idx = 0;

  // Copy the name (first 8 bytes), skipping trailing spaces
  for (int i = 0; i < 8; i++) {
    if (name[i] != ' ') {
      out[out_idx++] = name[i];
    }
  }

  // check if there is an extension (last 3 bytes)
  if (name[8] != ' ') {
    out[out_idx++] = '.';
    for (int i = 8; i < 11; i++) {
      if (name[i] != ' ') {
        out[out_idx++] = name[i];
      }
    }
  }

  out[out_idx] = '\0'; // null-terminate the string

  console_print(out);
  console_print("\n");
}

void fat32_list_root() {
  uint8_t buffer[512];

  uint32_t cluster = fs.root_cluster;

  while (cluster < 0x0FFFFFF8) {
    uint32_t sector = cluster_to_sector(cluster);

    disk_read(sector, buffer);

    fat32_dir_entry *entry = (fat32_dir_entry *)buffer;

    for (int i = 0; i < 16; i++) {
      if (entry[i].name[0] == 0x00)
        return;

      if (!(entry[i].attr & 0x0F))
        print_name(entry[i].name);
    }

    cluster = fat32_next_cluster(cluster);
  }
}

void fat32_read_file(fat32_dir_entry *entry, uint8_t *destination) {
  //  Get the starting cluster (combine high and low 16-bit words)
  uint32_t cluster = entry->first_cluster_high << 16 | entry->first_cluster_low;
  uint32_t bytes_remaining = entry->file_size;
  // temporary buffer for one cluster
  uint8_t cluster_buffer[512 * fs.sectors_per_cluster];

  // loop through the cluster chain
  while (cluster >= 2 && cluster < 0x0FFFFFF8) {
    uint32_t sector = cluster_to_sector(cluster);

    // Read all sectors in this cluster
    for (uint8_t i = 0; i < fs.sectors_per_cluster; i++) {
      disk_read(sector + i, destination);

      // move destination pointer forward by one sector
      destination += 512;

      // Safety check for file size
      if (bytes_remaining > 512) {
        bytes_remaining -= 512;
      } else {
        bytes_remaining = 0;
        return; // finished reading file
      }
    }

    // move to the next cluster
    cluster = fat32_next_cluster(cluster);
  }
}

void format_filename(const char *input, char *output) {
  // fill with spaces initially
  for (int i = 0; i < 11; i++) {
    output[i] = ' ';
  }

  int i = 0;
  int out_idx = 0;

  // copy name part
  while (input[i] != '.' && input[i] != '\0' && out_idx < 8) {
    char c = input[i++];
    if (c >= 'a' && c <= 'z') {
      c -= 32; // convert to uppercase
    }
    output[out_idx++] = c;
  }

  // move to extension if it exists
  if (input[i] == '.') {
    i++;
    out_idx = 8;
    while (input[i] != '\0' && out_idx < 11) {
      char c = input[i++];
      if (c >= 'a' && c <= 'z') {
        c -= 32; // convert to uppercase
      }
      output[out_idx++] = c;
    }
  }
}

fat32_dir_entry* fat32_find_file(const char* filename) {
    static fat32_dir_entry found_entry; // Static so it persists after return
    char search_name[11];
    format_filename(filename, search_name);

    uint8_t buffer[512];
    uint32_t cluster = fs.root_cluster;

    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        uint32_t sector = cluster_to_sector(cluster);
        disk_read(sector, buffer);

        fat32_dir_entry *entries = (fat32_dir_entry *)buffer;

        for (int i = 0; i < 16; i++) {
            // 0x00 means end of directory
            if (entries[i].name[0] == 0x00) return 0;

            // 0xE5 means the file was deleted
            if (entries[i].name[0] == 0xE5) continue;

            // Check if it's a normal file/dir (ignore Long File Name entries)
            if (entries[i].attr == 0x0F) continue;

            // Compare 11 bytes of the name
            int match = 1;
            for (int j = 0; j < 11; j++) {
                if (entries[i].name[j] != search_name[j]) {
                    match = 0;
                    break;
                }
            }

            if (match) {
                found_entry = entries[i];
                return &found_entry;
            }
        }
        cluster = fat32_next_cluster(cluster);
    }
    return 0; // Not found
}

void load_and_run_file(const char* name) {
    fat32_dir_entry* file = fat32_find_file(name);

    if (file) {
        console_print("File found! Loading...\n");

        // Allocate or point to a buffer large enough for file->file_size
        uint8_t* load_address = (uint8_t*)0x100000;

        fat32_read_file(file, load_address);

        console_print("Read complete.\n");
    } else {
        console_print("File not found.\n");
    }
}

void fat32_cat(const char *filename) {
    fat32_dir_entry *entry = fat32_find_file(filename);
    if (!entry) {
        console_print("File not found.\n");
        return;
    }

    uint32_t cluster = ((uint32_t)entry->first_cluster_high << 16) | entry->first_cluster_low;
    uint32_t bytes_remaining = entry->file_size;
    uint8_t sector_buffer[513]; // 512 + 1 for null terminator

    while (cluster >= 2 && cluster < 0x0FFFFFF8 && bytes_remaining > 0) {
        uint32_t sector = cluster_to_sector(cluster);

        // Read each sector in the cluster
        for (uint8_t i = 0; i < fs.sectors_per_cluster && bytes_remaining > 0; i++) {
            disk_read(sector + i, sector_buffer);

            // Determine how many bytes to print from this sector
            uint32_t to_print = (bytes_remaining > 512) ? 512 : bytes_remaining;

            // Null-terminate the buffer safely for console_print
            // Note: This only works if console_print stops at \0
            uint8_t temp = sector_buffer[to_print];
            sector_buffer[to_print] = '\0';

            console_print((char*)sector_buffer);

            // Restore the byte and update counter
            sector_buffer[to_print] = temp;
            bytes_remaining -= to_print;
        }

        cluster = fat32_next_cluster(cluster);
    }
    console_print("\n");
}
