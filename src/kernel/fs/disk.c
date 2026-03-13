#include <stdint.h>
#include "disk.h"
#include "ata.h"


void disk_read(uint32_t sector, uint8_t *buffer) {
  ata_read_sector(sector, buffer);
}

void disk_write(uint32_t sector, uint8_t *buffer) {
  ata_write_sector(sector, buffer);
}
