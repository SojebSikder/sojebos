#include <stdint.h>

extern void ata_read_sector(uint32_t lba, uint8_t *buffer);

void disk_read(uint32_t sector, uint8_t *buffer)
{
    ata_read_sector(sector, buffer);
}
