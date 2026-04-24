#include "ipv4.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"
#include "../memory/memory.h"
#include "../libc/string.h"

#define MAKE_IP(a,b,c,d) ((uint32_t)((a) | (b) << 8 | (c) << 16 | (d) << 24))

// Global IP for the OS (referenced by arp.c)
uint32_t sojeb_os_ip = MAKE_IP(192, 168, 10, 11);

/**
 * Standard Internet Checksum (RFC 1071)
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
 * Handle incoming IPv4 packets
 */
void ipv4_handle_packet(void *data, uint32_t len) {
    if (len < sizeof(struct ipv4_header)) return;

    struct ipv4_header *ip = (struct ipv4_header *)data;

    // Filter: Check if the destination is our IP or global broadcast
    if (ip->dest_ip != sojeb_os_ip && ip->dest_ip != 0xFFFFFFFF) {
        return;
    }

    // Verify Checksum (Should result in 0 if correct)
    // Note: Some drivers/emulators offload this, but we check in software for safety.

    uint32_t header_len = ip->ihl * 4;
    void *payload = (uint8_t *)data + header_len;

    // total_length is big-endian
    uint16_t total_len_host = __builtin_bswap16(ip->total_length);
    if (total_len_host < header_len) return;

    uint32_t payload_len = total_len_host - header_len;

    // Route based on protocol
    switch (ip->protocol) {
        case IPV4_PROTO_ICMP:
            icmp_handle(ip, payload, payload_len);
            break;

        // case IPV4_PROTO_UDP:
        //     udp_handle(ip, payload, payload_len);
        //     break;

        default:
            // Protocol not supported
            break;
    }
}

/**
 * Send an IPv4 packet
 */
void ipv4_send(uint32_t dest_ip, uint8_t protocol, void *data, uint32_t len) {
    uint32_t header_len = sizeof(struct ipv4_header);
    uint32_t total_len = header_len + len;

    // Allocate buffer for the whole IP packet
    uint8_t *packet = (uint8_t *)kmalloc(total_len);
    if (!packet) return;

    struct ipv4_header *ip = (struct ipv4_header *)packet;

    // Build the Header
    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->total_length = __builtin_bswap16(total_len);
    ip->id = __builtin_bswap16(0); // Fragmentation not implemented
    ip->fragment_offset = 0;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->src_ip = sojeb_os_ip;
    ip->dest_ip = dest_ip;

    // Calculate Checksum
    ip->checksum = 0;
    ip->checksum = net_checksum(ip, header_len);

    // Attach Payload
    memcpy(packet + header_len, data, len);

    // Resolve Destination MAC via ARP
    uint8_t *dest_mac = arp_lookup(dest_ip);

    if (dest_mac) {
        ethernet_send_packet(dest_mac, ETHERTYPE_IPV4, packet, total_len);
    } else {
        // We don't have the MAC. Trigger ARP request.
        // The current packet is dropped (standard behavior for simple stacks).
        // The upper layer/user will likely retry.
        arp_send_request(dest_ip);
    }

    // Cleanup
    kfree(packet);
}
