// ipv4 implementation

#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>

struct ipv4_header {
  uint8_t ihl : 4;       // internet header length
  uint8_t version : 4;   // usually 4
  uint8_t tos;           // type of service
  uint16_t total_length; // total length of the packet
  uint16_t id;           // identification of the packet
  uint16_t fragment_offset;
  uint8_t ttl;      // time to live
  uint8_t protocol; // protocol type: 1 = ICMP, 6 = TCP, 17 = UDP
  uint16_t checksum;
  uint32_t src_ip;
  uint32_t dest_ip;
} __attribute__((packed));

void ipv4_handle_packet(void *data, uint32_t len);
void ipv4_send(uint32_t dest_ip, uint8_t protocol, void *data, uint32_t len);
uint16_t net_checksum(void *vdata, uint32_t length);

#endif
