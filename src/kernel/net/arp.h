#ifndef ARP_H
#define ARP_H

#include <stdint.h>

#define ARP_HTYPE_ETHERNET 1
#define ARP_PTYPE_IPV4 0x0800
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY 2

struct arp_packet {
  uint16_t htype; // hardware type (Ethernet = 1)
  uint16_t ptype; // protocol type (IPv4 = 0x0800)
  uint8_t hlen;   // hardware address length (MAC = 6)
  uint8_t plen;   // protocol address length (IP = 4)
  uint16_t oper;  // operation (1 = request, 2 = reply)

  uint8_t src_mac[6];
  uint32_t src_ip;

  uint8_t dest_mac[6];
  uint32_t dest_ip;

} __attribute__((packed));

void arp_handle_packet(void *data, uint32_t len);
void arp_send_request(uint32_t target_ip);
uint8_t *arp_lookup(uint32_t ip);

#endif
