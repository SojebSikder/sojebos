#ifndef PCI_H
#define PCI_H

#include <stdint.h>

uint16_t pci_read_word(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset);
uint32_t pci_read_long(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset);
uint32_t pci_find_rtl8139();

#endif
