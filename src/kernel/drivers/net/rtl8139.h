// This driver handles the "Zero to Raw packet" phase

#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>

// registres
#define RTL8139_REG_MAC 0x00
#define RTL8139_REG_MAR 0x08
#define RTL8139_REG_TXSTATUS 0x10
#define RTL8139_REG_TXADDR 0x20
#define RTL8139_REG_RBSTART 0x30
#define RTL8139_REG_COMMAND 0x37
#define RTL8139_REG_CAPR 0x38
#define RTL8139_REG_IMR 0x3C
#define RTL8139_REG_ISR 0x3E
#define RTL8139_REG_CONFIG1 0x52

void rtl8139_init(uint32_t pci_io_base);
void rtl8139_send_packet(void *data, uint32_t len);
void rtl8139_handler();
void rtl8139_poll();

#endif
