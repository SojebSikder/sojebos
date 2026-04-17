#include "pci.h"
#include "io.h"

uint32_t pci_read_long(uint16_t bus, uint16_t slot, uint16_t func,
                       uint16_t offset) {
  uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) |
                                (offset & 0xfc) | ((uint32_t)0x80000000));

  outl(0xCF8, address);
  return inl(0xCFC);
}

uint16_t pci_read_word(uint16_t bus, uint16_t slot, uint16_t func,
                       uint16_t offset) {
  uint32_t reg = pci_read_long(bus, slot, func, offset);
  return (uint16_t)((reg >> ((offset & 2) * 8)) & 0xffff);
}

uint32_t pci_find_rtl8139() {
  for (uint16_t bus = 0; bus < 256; bus++) {
    for (uint16_t slot = 0; slot < 32; slot++) {
      if (pci_read_word(bus, slot, 0, 0) == 0x10EC) {   // Realtek vendor
        if (pci_read_word(bus, slot, 0, 2) == 0x8139) { // RTL8139 device
          // Found RTL8139, return base address
          // get BAR0 (I/O base), mask bit 0 (I/O space indicator)
          return pci_read_long(bus, slot, 0, 0x10) & ~0x1;
        }
      }
    }
  }
  return 0;
}
