#include <stdint.h>
#include "../io.h"


#define ATA_DATA       0x1F0
#define ATA_SECTOR_CNT 0x1F2
#define ATA_LBA_LOW    0x1F3
#define ATA_LBA_MID    0x1F4
#define ATA_LBA_HIGH   0x1F5
#define ATA_DRIVE      0x1F6
#define ATA_CMD        0x1F7
#define ATA_STATUS     0x1F7


static void ata_wait()
{
    while (inb(ATA_STATUS) & 0x80);
}

void ata_read_sector(uint32_t lba, uint8_t *buffer) // Changed to pointer
{
    ata_wait();
    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LOW, (uint8_t)lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_CMD, 0x20);

    ata_wait();

    uint16_t *ptr = (uint16_t *)buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(ATA_DATA);
    }
}
