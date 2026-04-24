#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP  0x0806

struct eth_header {
    uint8_t  dest_mac[6];
    uint8_t  src_mac[6];
    uint16_t type;
} __attribute__((packed));

/**
 * @brief Processes an incoming raw ethernet frame.
 * @param data Pointer to the start of the ethernet header.
 * @param len Total length of the frame (including header).
 */
void ethernet_handle_packet(void* data, uint32_t len);

/**
 * @brief Encapsulates payload into an Ethernet frame and sends it.
 * @param dest_mac The MAC address of the destination.
 * @param type The EtherType (e.g., ETHERTYPE_IPV4).
 * @param data The payload data (IP packet, ARP message, etc).
 * @param len The length of the payload (excluding Ethernet header).
 */
void ethernet_send_packet(uint8_t* dest_mac, uint16_t type, void* data, uint32_t len);

#endif
