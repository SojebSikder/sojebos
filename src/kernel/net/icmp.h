#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>
#include "ipv4.h"

#define ICMP_TYPE_ECHO_REPLY   0
#define ICMP_TYPE_ECHO_REQUEST 8

struct icmp_header {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed));

/**
 * Handle an incoming ICMP packet.
 * Called by ipv4_handle_packet when protocol == 1.
 */
void icmp_handle(struct ipv4_header* ip_hdr, void* data, uint32_t len);

#endif
