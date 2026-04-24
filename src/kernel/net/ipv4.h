#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>

#define IPV4_PROTO_ICMP 1
#define IPV4_PROTO_TCP  6
#define IPV4_PROTO_UDP  17

struct ipv4_header {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint8_t  ihl : 4;       // Header length (typically 5)
    uint8_t  version : 4;   // Version (typically 4)
#else
    uint8_t  version : 4;
    uint8_t  ihl : 4;
#endif
    uint8_t  tos;           // Type of Service
    uint16_t total_length;  // Total length of (header + payload)
    uint16_t id;            // Identification
    uint16_t fragment_offset; // Flags (3 bits) + Fragment Offset (13 bits)
    uint8_t  ttl;           // Time to Live
    uint8_t  protocol;      // Protocol (ICMP=1, TCP=6, UDP=17)
    uint16_t checksum;      // Header checksum
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed));

void ipv4_handle_packet(void *data, uint32_t len);
void ipv4_send(uint32_t dest_ip, uint8_t protocol, void *data, uint32_t len);
uint16_t net_checksum(void *vdata, uint32_t length);

#endif
