#include <stdint.h>

void disk_read(uint32_t sector, uint8_t *buffer);
void disk_write(uint32_t sector, uint8_t *buffer);
