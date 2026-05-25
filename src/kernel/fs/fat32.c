#include "fat32.h"
#include "../drivers/console.h"
#include "../libc/string.h"
#include "disk.h"
#include <stdint.h>

// sector size in bytes
#define SECTOR_SIZE 512

static FAT32 fs;
static uint32_t
    current_dir_cluster; // tracks the current active directory cluster

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

  // default current working directory to root cluster
  current_dir_cluster = fs.root_cluster;
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

void fat32_ls() {
  uint8_t buffer[512];
  uint32_t cluster = current_dir_cluster;

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
  uint32_t cluster = current_dir_cluster;

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
  uint32_t dir_cluster = current_dir_cluster;
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
  uint32_t cluster = current_dir_cluster;

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
      if (i == 0 && j < 2)
        continue;
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

//
// Directory operations
//

// this is the attribute value for a directory entry
#define ATTR_DIRECTORY 0x10

static void init_directory_cluster(uint32_t current_cluster,
                                   uint32_t parent_cluster) {
  uint8_t buffer[512];
  // clear the sector buffer first
  memset(buffer, 0, 512);

  fat32_dir_entry *entries = (fat32_dir_entry *)buffer;

  // create '.' entry
  for (int i = 0; i < 11; i++) {
    entries[0].name[i] = ' ';
  }
  entries[0].name[0] = '.';
  entries[0].attr = ATTR_DIRECTORY;
  entries[0].first_cluster_low = current_cluster & 0xFFFF;
  entries[0].first_cluster_high = (current_cluster >> 16) & 0xFFFF;
  entries[0].file_size = 0;

  // create '..' entry
  for (int i = 0; i < 11; i++) {
    entries[1].name[i] = ' ';
  }
  entries[1].name[0] = '.';
  entries[1].name[1] = '.';
  entries[1].attr = ATTR_DIRECTORY;

  // if the parent is the root directory, FAT32 specs state its cluster should
  // be set to 0
  uint32_t p_cluster = (parent_cluster == fs.root_cluster) ? 0 : parent_cluster;
  entries[1].first_cluster_low = p_cluster & 0xFFFF;
  entries[1].first_cluster_high = (p_cluster >> 16) & 0xFFFF;
  entries[1].file_size = 0;

  // write this initialization buffer to the first sector of the new cluster
  uint32_t start_sector = cluster_to_sector(current_cluster);
  disk_write(start_sector, buffer);

  // zero out the remaining sectors in this cluster so old disk data doesn't
  // corrupt it
  memset(buffer, 0, 512);
  for (uint8_t s = 1; s < fs.sectors_per_cluster; s++) {
    disk_write(start_sector + s, buffer);
  }
}

void fat32_create_directory(const char *dirname) {
  // ensure it doesn't already exist
  if (fat32_find_file(dirname) != 0) {
    console_print("Directory or file already exists");
    return;
  }

  // find and reserve a free cluster for the new directory data block
  uint32_t new_cluster = fat32_find_free_cluster();
  if (new_cluster == 0x0FFFFFFF) {
    console_print("Error: Disk Full.\n");
    return;
  }
  fat32_set_next_cluster(new_cluster, 0x0FFFFFFF); // mark as end of chain

  // populate '.' and '..' inside this new cluster
  // TODO: assuming creation inside Root Directory for simplicity,
  // should adjust if creating in a subdirectory
  init_directory_cluster(new_cluster, current_dir_cluster);

  // find an empty slot in the parent directory (Root) to record this new
  // directory
  uint32_t dir_cluster = current_dir_cluster;
  uint8_t dir_buffer[512];

  while (dir_cluster >= 2 && dir_cluster < 0x0FFFFFF8) {
    uint32_t sector_start = cluster_to_sector(dir_cluster);

    for (uint8_t s = 0; s < fs.sectors_per_cluster; s++) {
      disk_read(sector_start + s, dir_buffer);
      fat32_dir_entry *entries = (fat32_dir_entry *)dir_buffer;

      for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00 || (uint8_t)entries[i].name[0] == 0xE5) {
          // format and save directory configuration
          format_filename(dirname, entries[i].name);
          entries[i].attr = ATTR_DIRECTORY;
          entries[i].first_cluster_low = new_cluster & 0xFFFF;
          entries[i].first_cluster_high = (new_cluster >> 16) & 0xFFFF;
          entries[i].file_size = 0;

          disk_write(sector_start + s, dir_buffer);
          console_print("Directory created successfully.\n");
          return;
        }
      }
    }
  }
}

static int is_directory_empty(uint32_t dir_cluster) {
  uint8_t buffer[512];
  uint32_t cluster = dir_cluster;

  while (cluster >= 2 && cluster < 0x0FFFFFF8) {
    uint32_t sector_start = cluster_to_sector(cluster);

    for (uint8_t s = 0; s < fs.sectors_per_cluster; s++) {
      disk_read(sector_start + s, buffer);
      fat32_dir_entry *entries = (fat32_dir_entry *)buffer;

      for (int i = 0; i < 16; i++) {
        // if it's a completely unallocated slot, we can stop evaluating
        if (entries[i].name[0] == 0x00) {
          continue;
        }
        if ((uint8_t)entries[i].name[0] == 0xE5) {
          continue; // skip deleted items
        }
        if (entries[i].attr == 0x0F) {
          continue; // skip LFN
        }

        // if name is "." or "..", it's a standard boiler-plate directory logic
        if (entries[i].name[0] == '.' &&
            (entries[i].name[1] == ' ' ||
             (entries[i].name[1] == '.' && entries[i].name[2] == ' '))) {
          continue;
        }

        // if we hit any other active file/folder, the directory isn't empty
        return 0;
      }
    }
    cluster = fat32_next_cluster(cluster);
  }
  return 1;
}

void fat32_rmdir(const char *dirname) {
  char search_name[11];
  format_filename(dirname, search_name);

  uint8_t dir_buffer[512];
  uint32_t cluster = current_dir_cluster;

  while (cluster >= 2 && cluster < 0x0FFFFFF8) {
    uint32_t sector_start = cluster_to_sector(cluster);

    for (uint8_t s = 0; s < fs.sectors_per_cluster; s++) {
      uint32_t current_sector = sector_start + s;
      disk_read(current_sector, dir_buffer);
      fat32_dir_entry *entries = (fat32_dir_entry *)dir_buffer;

      for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00) {
          return; // directoryt end
        }
        if ((uint8_t)entries[i].name[0] == 0xE5) {
          continue;
        }

        // match name string
        int match = 1;
        for (int j = 0; j < 11; j++) {
          if (entries[i].name[j] != search_name[j]) {
            match = 0;
            break;
          }
        }

        if (match) {
          // check if it's actually a directory
          if ((entries[i].attr & 0x10) == 0) {
            console_print("Error: Target path is a file, not a directory.\n");
            return;
          }

          uint32_t cluster_to_free =
              ((uint32_t)entries[i].first_cluster_high << 16) |
              entries[i].first_cluster_low;

          // check if it's empty
          if (!is_directory_empty(cluster_to_free)) {
            console_print("Error: Directory is not empty.\n");
            return;
          }

          // clear the FAT space allocation
          while (cluster_to_free >= 2 && cluster_to_free < 0x0FFFFFF8) {
            uint32_t next = fat32_next_cluster(cluster_to_free);
            fat32_set_next_cluster(cluster_to_free, 0x00000000);
            cluster_to_free = next;
          }

          // flag directory record status as deleted
          entries[i].name[0] = 0xE5;
          disk_write(current_sector, dir_buffer);
          console_print("Directory deleted successfully.\n");
          return;
        }
      }
    }
    cluster = fat32_next_cluster(cluster);
  }
}

// helper function to extract the next token up to a '/'
static int get_next_component(const char *path, int start_idx,
                              char *component) {
  int i = start_idx;
  // skip leading slashes
  while (path[i] == '/') {
    i++;
  }

  int c_idx = 0;
  while (path[i] != '/' && path[i] != '\0' && c_idx < 127) {
    component[c_idx++] = path[i++];
  }
  component[c_idx] = '\0';
  return i; // return the next index to continue parsing
}

// travarses a path string and returns the final directory's cluster number
// returns 0xFFFFFFFF if the path is invalid or not found
uint32_t fat32_get_path_cluster(const char *path) {
  uint32_t cluster = current_dir_cluster;
  int idx = 0;

  // absolute path resolution check
  if (path[0] == '/') {
    cluster = fs.root_cluster;
    idx = 1;
  }

  char component[128];
  char formatted_search[11];
  uint8_t buffer[512];

  while (path[idx] != '\0') {
    idx = get_next_component(path, idx, component);
    if (component[0] == '\0') {
      break; // no more components to process
    }

    format_filename(component, formatted_search);
    int found = 0;
    uint32_t run_cluster = cluster;

    // search the current directory for this matching component
    while (run_cluster >= 2 && run_cluster < 0x0FFFFFF8 && !found) {
      uint32_t sector_start = cluster_to_sector(run_cluster);

      for (uint8_t s = 0; s < fs.sectors_per_cluster; s++) {
        disk_read(sector_start + s, buffer);
        fat32_dir_entry *entries = (fat32_dir_entry *)buffer;

        for (int i = 0; i < 16; i++) {
          if (entries[i].name[0] == 0x00) {
            break; // end of directory
          }
          if ((uint8_t)entries[i].name[0] == 0xE5) {
            continue;
          }
          if (entries[i].attr == 0x0F) {
            continue; // skip LFN entries
          }

          if (memcmp(entries[i].name, formatted_search, 11) == 0) {
            // must be a directory
            if ((entries[i].attr & ATTR_DIRECTORY) != 0) {
              cluster = ((uint32_t)entries[i].first_cluster_high << 16) |
                        entries[i].first_cluster_low;

              // Root directory target cluster inside '.' or '..'
              // is always written as 0
              if (cluster == 0) {
                cluster = fs.root_cluster;
              }
              found = 1;
              break;
            }
          }
        }
        if (found) {
          break;
        }
      }
      if (found) {
        break;
      }
      run_cluster = fat32_next_cluster(run_cluster);
    }
    if (!found) {
      return 0xFFFFFFFF; // part of the path segment was not found
    }
  }
  return cluster;
}

void fat32_cd(const char *path) {
  uint32_t target_cluster = fat32_get_path_cluster(path);
  if (target_cluster == 0xFFFFFFFF) {
    console_print("cd: No such file or directory.\n");
  } else {
    current_dir_cluster = target_cluster;
  }
}

static int find_dirname_in_parent(uint32_t parent_cluster,
                                  uint32_t child_cluster, char *out_name) {
  uint8_t buffer[512];
  uint32_t cluster = parent_cluster;

  while (cluster >= 2 && cluster < 0xFFFFFFF8) {
    uint32_t sector_start = cluster_to_sector(cluster);

    for (uint8_t s = 0; s < fs.sectors_per_cluster; s++) {
      disk_read(sector_start + s, buffer);
      fat32_dir_entry *entries = (fat32_dir_entry *)buffer;

      for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0x00) {
          return 0; // end of directory
        }
        if ((uint8_t)entries[i].name[0] == 0xE5) {
          continue;
        }
        if (entries[i].attr == 0x0F) {
          continue; // skip LFN entries
        }

        // check if it's a directory entry matching out target cluster number
        if ((entries[i].attr & ATTR_DIRECTORY) != 0) {
          uint32_t entry_cluster =
              ((uint32_t)entries[i].first_cluster_high << 16) |
              entries[i].first_cluster_low;

          // FAT32 treats 0 as the root directory within dot entries
          if (entry_cluster == 0 && parent_cluster == fs.root_cluster) {
            entry_cluster = fs.root_cluster;
          }

          if (entry_cluster == child_cluster) {
            // extract name (up to 8 chars)
            int out_idx = 0;
            for (int k = 0; k < 8; k++) {
              if (entries[i].name[k] != ' ') {
                out_name[out_idx++] = entries[i].name[k];
              }
            }
            out_name[out_idx] = '\0';
            return 1;
          }
        }
      }
    }
    cluster = fat32_next_cluster(cluster);
  }
  return 0;
}

void fat32_pwd() {
  // if the current directory is the root directory
  if (current_dir_cluster == fs.root_cluster) {
    console_print("/\n");
    return;
  }

  // max depth of 16 clusters (subdirectories)
  uint32_t cluster_stack[16];
  int stack_idx = 0;

  uint32_t walk_cluster = current_dir_cluster;

  // climb up to the root directory using '..' records
  while (walk_cluster != fs.root_cluster && stack_idx < 16) {
    cluster_stack[stack_idx++] = walk_cluster;

    // read the current cluster's first sector to locate the '..' entry
    uint8_t buffer[SECTOR_SIZE];
    disk_read(cluster_to_sector(walk_cluster), buffer);
    fat32_dir_entry *entries = (fat32_dir_entry *)buffer;

    // verify that entries[1] is explicitly a '..' record
    if (entries[1].name[0] == '.' && entries[1].name[1] == '.') {
      uint32_t parent = ((uint32_t)entries[1].first_cluster_high << 16) |
                        entries[1].first_cluster_low;

      // per FAT32 spec, a parent value of 0 means the parent is the root
      // cluster
      if (parent == 0) {
        parent = fs.root_cluster;
      }

      // avoid infinite loops by checking if the parent is the current cluster
      if (parent == walk_cluster) {
        break;
      }
      walk_cluster = parent;
    } else {
      console_print("Error: Directory structure corrupted ('..' missing).\n");
      return;
    }
  }

  // trace down from root and look up string components names
  uint32_t parent = fs.root_cluster;
  char name_buffer[256];

  for (int i = stack_idx - 1; i >= 0; i--) {
    uint32_t child = cluster_stack[i];
    console_print("/");

    if (find_dirname_in_parent(parent, child, name_buffer)) {
      console_print(name_buffer);
    } else {
      console_print("???"); // fallback identifier for an orphaned cluster
    }
    parent = child;
  }
  console_print("\n");
}
