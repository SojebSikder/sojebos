#include <stdint.h>

extern void ata_read_sector(uint32_t lba, uint8_t *buffer);
extern void ata_write_sector(uint32_t lba, uint8_t *buffer);
