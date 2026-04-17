// ethernet implementation

#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP 0x0806

struct eth_header {
  uint8_t dest[6];
  uint8_t src[6];
  uint16_t type;
} __attribute__((packed));

void ethernet_handle_packet(void* data, uint32_t len);
void ethernet_send_packet(uint8_t* dest_mac, uint16_t type, void* data, uint32_t len);

#endif
