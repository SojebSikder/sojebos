#include "ethernet.h"
#include "ipv4.h"

void ethernet_handle_packet(void* data, uint32_t len) {
    struct eth_header* eth = (struct eth_header*)data;

    // Convert Big-Endian type to Host-Endian
    uint16_t type = __builtin_bswap16(eth->type);
    void* payload = (uint8_t*)data + sizeof(struct eth_header);
    uint32_t payload_len = len - sizeof(struct eth_header);

    if (type == ETHERTYPE_IPV4) {
        ipv4_handle_packet(payload, payload_len);
    }
    // else if (type == ETHERTYPE_ARP) { arp_handle(...); }
}
