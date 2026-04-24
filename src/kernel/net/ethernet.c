#include "ethernet.h"
#include "arp.h"
#include "ipv4.h"
#include "../drivers/net/rtl8139.h"
#include "../libc/string.h"

// The MAC address of our own network card (set during driver init)
extern uint8_t mac_address[6];

/**
 * Handle incoming Ethernet frames
 */
void ethernet_handle_packet(void* data, uint32_t len) {
    if (len < sizeof(struct eth_header)) return;

    struct eth_header* eth = (struct eth_header*)data;

    // Filter: Check if the packet is for us, or a broadcast
    // Broadcast MAC is FF:FF:FF:FF:FF:FF
    int is_broadcast = 1;
    int for_us = 1;

    for (int i = 0; i < 6; i++) {
        if (eth->dest_mac[i] != 0xFF) is_broadcast = 0;
        if (eth->dest_mac[i] != mac_address[i]) for_us = 0;
    }

    if (!is_broadcast && !for_us) {
        return; // Drop packet (Promiscuous mode might have picked it up)
    }

    // Extract payload
    uint16_t type = __builtin_bswap16(eth->type);
    void* payload = (uint8_t*)data + sizeof(struct eth_header);
    uint32_t payload_len = len - sizeof(struct eth_header);

    // Route to sub-protocols
    switch (type) {
        case ETHERTYPE_IPV4:
            ipv4_handle_packet(payload, payload_len);
            break;

        case ETHERTYPE_ARP:
            arp_handle_packet(payload, payload_len);
            break;

        default:
            // Unknown protocol, ignore
            break;
    }
}

/**
 * Encapsulate and send a packet
 */
void ethernet_send_packet(uint8_t* dest_mac, uint16_t type, void* data, uint32_t len) {
    struct eth_header header;

    // Fill the Ethernet header
    memcpy(header.dest_mac, dest_mac, 6);
    memcpy(header.src_mac, mac_address, 6);
    header.type = __builtin_bswap16(type);

    // Create a temporary buffer for the whole frame
    // In a real OS, you'd use a specialized "packet buffer" (sk_buff / mbuf)
    uint8_t frame[1514];
    memcpy(frame, &header, sizeof(struct eth_header));
    memcpy(frame + sizeof(struct eth_header), data, len);

    // Pass to the physical driver
    rtl8139_send_packet(frame, len + sizeof(struct eth_header));
}
