#include "ipv4.h"
#include "ethernet.h"
#include "icmp.h"
#include "../memory/memory.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include <stdint.h>

#define MAKE_IP(a,b,c,d) ((uint32_t)((a) | (b) << 8 | (c) << 16 | (d) << 24))

// Static IP for SojebOS.
// Change this to match your virtual network (e.g., 10.0.2.15 for QEMU user net)
uint32_t sojeb_os_ip = MAKE_IP(192, 168, 10, 11);

/**
 * Standard IPv4 Checksum (1's complement)
 */
uint16_t net_checksum(void *vdata, uint32_t length) {
    uint32_t sum = 0;
    uint16_t *data = (uint16_t *)vdata;

    while (length > 1) {
        sum += *data++;
        length -= 2;
    }

    if (length > 0) {
        sum += *(uint8_t *)data;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~((uint16_t)sum);
}

/**
 * Incoming IP packet handler
 */
void ipv4_handle_packet(void *data, uint32_t len) {
    struct ipv4_header *ip = (struct ipv4_header *)data;

    // Minimum sanity check
    if (len < sizeof(struct ipv4_header)) return;

    // Filter: Only process packets meant for SojebOS IP or Broadcast
    if (ip->dest_ip != sojeb_os_ip && ip->dest_ip != 0xFFFFFFFF) {
        return;
    }

    uint32_t header_len = ip->ihl * 4;
    void *payload = (uint8_t *)data + header_len;
    uint32_t payload_len = __builtin_bswap16(ip->total_length) - header_len;

    if (ip->protocol == 1) { // ICMP Protocol
        icmp_handle(ip, payload, payload_len);
    }
}

/**
 * Outgoing IP packet constructor
 */
void ipv4_send(uint32_t dest_ip, uint8_t protocol, void *data, uint32_t len) {
    uint32_t total_len = sizeof(struct ipv4_header) + len;

    // Allocate a temporary buffer for the combined packet
    uint8_t *combined_packet = (uint8_t *)kmalloc(total_len);
    if (!combined_packet) return;

    struct ipv4_header *ip = (struct ipv4_header *)combined_packet;

    // Fill the IP Header
    ip->version = 4;
    ip->ihl = 5; // 5 * 4 = 20 bytes
    ip->tos = 0;
    ip->total_length = __builtin_bswap16(total_len);
    ip->id = __builtin_bswap16(1);
    ip->fragment_offset = 0;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->src_ip = sojeb_os_ip;
    ip->dest_ip = dest_ip;

    // Calculate Checksum (Must be 0 before calculation)
    ip->checksum = 0;
    ip->checksum = net_checksum(ip, sizeof(struct ipv4_header));

    // Copy the payload (data) into the buffer after the header
    memcpy(combined_packet + sizeof(struct ipv4_header), data, len);

    //  Send to Ethernet layer
    // NOTE: Without ARP, we broadcast the MAC.
    // The recipient will see the IP matches and process it.
    uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    // TODO: implement this
    // ethernet_send_packet(broadcast_mac, ETHERTYPE_IPV4, combined_packet, total_len);

    // 6. Free the temporary buffer
    kfree(combined_packet);
}
