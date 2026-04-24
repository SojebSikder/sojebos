#include "fat32.h"
#include "../drivers/console.h"
#include "../libc/mem.h"
#include "disk.h"
#include <stdint.h>
#include "../libc/string.h"

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
  if ((unsigned char)name[0] == 0xE5) {
    return;
  }

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

  while (cluster >= 2 && cluster < 0x0FFFFFF8) {
    uint32_t sector_start = cluster_to_sector(cluster);

    for (uint8_t s = 0; s < fs.sectors_per_cluster; s++) {
      disk_read(sector_start + s, buffer);
      fat32_dir_entry *entries = (fat32_dir_entry *)buffer;

      for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00) {
          return; // End of directory
        }
        if ((uint8_t)entries[i].name[0] == 0xE5) {
          continue; // Skip deleted
        }
        if (entries[i].attr == 0x0F) {
          continue; // Skip LFN entries
        }

        print_name(entries[i].name);
      }
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

fat32_dir_entry *fat32_find_file(const char *filename) {
  static fat32_dir_entry found_entry;
  char search_name[11];
  format_filename(filename, search_name);

  uint8_t buffer[512];
  uint32_t cluster = fs.root_cluster;

  while (cluster >= 2 && cluster < 0x0FFFFFF8) {
    uint32_t sector_base = cluster_to_sector(cluster);

    // Loop through every sector in the cluster!
    for (uint8_t s = 0; s < fs.sectors_per_cluster; s++) {
      disk_read(sector_base + s, buffer);
      fat32_dir_entry *entries = (fat32_dir_entry *)buffer;

      for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00)
          return 0;
        if ((uint8_t)entries[i].name[0] == 0xE5)
          continue;
        if (entries[i].attr == 0x0F)
          continue;

        if (memcmp(entries[i].name, search_name, 11) == 0) {
          found_entry = entries[i];
          // To make delete work, we actually need the
          // sector number and index, not just the entry copy.
          return &found_entry;
        }
      }
    }
    cluster = fat32_next_cluster(cluster);
  }
  return 0;
}

void load_and_run_file(const char *name) {
  fat32_dir_entry *file = fat32_find_file(name);

  if (file) {
    console_print("File found! Loading...\n");

    // Allocate or point to a buffer large enough for file->file_size
    uint8_t *load_address = (uint8_t *)0x100000;

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

  uint32_t cluster =
      ((uint32_t)entry->first_cluster_high << 16) | entry->first_cluster_low;
  uint32_t bytes_remaining = entry->file_size;
  uint8_t sector_buffer[513]; // 512 + 1 for null terminator

  while (cluster >= 2 && cluster < 0x0FFFFFF8 && bytes_remaining > 0) {
    uint32_t sector = cluster_to_sector(cluster);

    // Read each sector in the cluster
    for (uint8_t i = 0; i < fs.sectors_per_cluster && bytes_remaining > 0;
         i++) {
      disk_read(sector + i, sector_buffer);

      // Determine how many bytes to print from this sector
      uint32_t to_print = (bytes_remaining > 512) ? 512 : bytes_remaining;

      // Null-terminate the buffer safely for console_print
      // This only works if console_print stops at \0
      uint8_t temp = sector_buffer[to_print];
      sector_buffer[to_print] = '\0';

      console_print((char *)sector_buffer);

      // Restore the byte and update counter
      sector_buffer[to_print] = temp;
      bytes_remaining -= to_print;
    }

    cluster = fat32_next_cluster(cluster);
  }
  console_print("\n");
}

//
// Write disk
//

// find a free cluster in the FAT (entry == 0)
uint32_t fat32_find_free_cluster() {
  uint8_t buffer[512];
  uint32_t fat_sector_start = fs.reserved_sector_count;

  for (uint32_t i = 0; i < fs.fat_size; i++) {

    disk_read(fat_sector_start + i, buffer);
    uint32_t *entries = (uint32_t *)buffer;

    for (int j = 0; j < 128; j++) {
      // cluster 0 and 1 are reserved, start looking from cluster 2
      if (i == 0 && j < 2) {
        continue;
      }

      if ((entries[j] & 0x0FFFFFFF) == 0) {
        return (i * 128) + j;
      }
    }
  }
  return 0x0FFFFFFF; // No free space/cluster found
}

// update an entry in the FAT
void fat32_set_next_cluster(uint32_t cluster, uint32_t next_value) {
  uint8_t buffer[512];
  uint32_t fat_offset = cluster * 4;
  uint32_t fat_sector = fs.reserved_sector_count + (fat_offset / 512);
  uint32_t entry_offset = fat_offset % 512;

  disk_read(fat_sector, buffer);

  // preserve the high 4 bits as per FAT32 spec
  uint32_t current = *(uint32_t *)&buffer[entry_offset];
  next_value = (next_value & 0x0FFFFFFF) | (current & 0xF0000000);

  *(uint32_t *)&buffer[entry_offset] = next_value;
  disk_write(fat_sector, buffer);
}

void fat32_write_file(const char *filename, uint8_t *data, uint32_t size) {
  uint32_t bytes_per_cluster = fs.sectors_per_cluster * 512;
  uint32_t clusters_needed = (size + bytes_per_cluster - 1) / bytes_per_cluster;

  uint32_t prev_cluster = 0;
  uint32_t first_cluster = 0;

  // Allocate and link clusters in the FAT
  for (uint32_t i = 0; i < clusters_needed; i++) {
    uint32_t current_cluster = fat32_find_free_cluster();
    if (current_cluster == 0x0FFFFFFF)
      return;

    if (i == 0) {
      first_cluster = current_cluster;
    } else {
      fat32_set_next_cluster(prev_cluster, current_cluster);
    }

    fat32_set_next_cluster(current_cluster, 0x0FFFFFFF); // Mark EOF

    // Write data to sectors
    uint32_t start_sector = cluster_to_sector(current_cluster);
    for (uint8_t s = 0; s < fs.sectors_per_cluster; s++) {
      uint32_t data_offset = (i * bytes_per_cluster) + (s * 512);
      if (data_offset < size) {
        uint32_t chunk_size = size - data_offset;
        if (chunk_size >= 512) {
          disk_write(start_sector + s, data + data_offset);
        } else {
          uint8_t padding[512] = {0};
          for (uint32_t j = 0; j < chunk_size; j++)
            padding[j] = data[data_offset + j];
          disk_write(start_sector + s, padding);
        }
      }
    }
    prev_cluster = current_cluster;
  }

  // Find a slot in the Root Directory (Iterating clusters and sectors)
  uint32_t dir_cluster = fs.root_cluster;
  uint8_t dir_buffer[512];

  while (dir_cluster >= 2 && dir_cluster < 0x0FFFFFF8) {
    uint32_t sector_start = cluster_to_sector(dir_cluster);

    for (uint8_t s = 0; s < fs.sectors_per_cluster; s++) {
      disk_read(sector_start + s, dir_buffer);
      fat32_dir_entry *entries = (fat32_dir_entry *)dir_buffer;

      for (int i = 0; i < 16; i++) {
        // If the entry is empty (0x00) or deleted (0xE5), we take it
        if (entries[i].name[0] == 0x00 || (uint8_t)entries[i].name[0] == 0xE5) {
          format_filename(filename, entries[i].name);
          entries[i].attr = 0x20; // Archive attribute
          entries[i].first_cluster_low = first_cluster & 0xFFFF;
          entries[i].first_cluster_high = (first_cluster >> 16) & 0xFFFF;
          entries[i].file_size = size;

          disk_write(sector_start + s, dir_buffer);
          return; // Success
        }
      }
    }
    dir_cluster = fat32_next_cluster(dir_cluster);
    // If dir_cluster becomes EOF here, we technically need to
    // allocate a NEW cluster for the directory, but for a basic OS,
    // 1 cluster (usually 4KB) is usually enough for 128 files.
  }
}

void fat32_delete_file(const char *filename) {
  char search_name[11];
  format_filename(filename, search_name);

  uint8_t dir_buffer[512];
  uint32_t cluster = fs.root_cluster;

  // Search for the file in the root directory
  while (cluster >= 2 && cluster < 0x0FFFFFF8) {
    uint32_t sector_start = cluster_to_sector(cluster);

    // Loop through every sector in the cluster
    for (uint8_t s = 0; s < fs.sectors_per_cluster; s++) {
      uint32_t current_sector = sector_start + s;
      disk_read(current_sector, dir_buffer);
      fat32_dir_entry *entries = (fat32_dir_entry *)dir_buffer;

      for (int i = 0; i < 16; i++) {
        // End of directory - if we hit this, the file definitely doesn't exist
        if (entries[i].name[0] == 0x00)
          return;

        // Skip already deleted entries
        if ((uint8_t)entries[i].name[0] == 0xE5)
          continue;

        // Compare names (11 bytes: 8 name + 3 extension)
        int match = 1;
        for (int j = 0; j < 11; j++) {
          if (entries[i].name[j] != search_name[j]) {
            match = 0;
            break;
          }
        }

        // Match found! (And not a Long File Name entry)
        if (match && entries[i].attr != 0x0F) {
          uint32_t cluster_to_free =
              ((uint32_t)entries[i].first_cluster_high << 16) |
              entries[i].first_cluster_low;

          // Clear the FAT chain (mark clusters as 0x00000000)
          while (cluster_to_free >= 2 && cluster_to_free < 0x0FFFFFF8) {
            uint32_t next = fat32_next_cluster(cluster_to_free);
            fat32_set_next_cluster(cluster_to_free, 0x00000000);
            cluster_to_free = next;
          }

          // Mark directory entry as deleted
          entries[i].name[0] = 0xE5;

          // Write ONLY this specific sector back to disk
          disk_write(current_sector, dir_buffer);

          // console_print("File deleted successfully.\n");
          return; // success
        }
      }
    }
    // Move to the next cluster in the directory chain if needed
    cluster = fat32_next_cluster(cluster);
  }
}

void fat32_show_usage() {
    uint8_t buffer[512];
    uint32_t free_clusters = 0;
    uint32_t fat_sector_start = fs.reserved_sector_count;

    console_print("Scanning Disk... ");

    for (uint32_t i = 0; i < fs.fat_size; i++) {
        disk_read(fat_sector_start + i, buffer);
        uint32_t *entries = (uint32_t *)buffer;

        for (int j = 0; j < 128; j++) {
            if (i == 0 && j < 2) continue;
            if ((entries[j] & 0x0FFFFFFF) == 0) {
                free_clusters++;
            }
        }
    }

    uint32_t free_kb = (free_clusters * fs.sectors_per_cluster) / 2;

    char str_buf[16];

    console_print("\nFree Clusters: ");
    itoa(free_clusters, str_buf);
    console_print(str_buf);

    console_print("\nFree Space: ");
    itoa(free_kb, str_buf);
    console_print(str_buf);
    console_print(" KB\n");
}
