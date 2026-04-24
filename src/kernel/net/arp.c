#include "arp.h"
#include "../libc/string.h"
#include "ethernet.h"
#include "ipv4.h"
#include <stdint.h>

// ARP cache (later we will move this to hash table)
#define ARP_CACHE_SIZE 32
struct arp_entry {
  uint32_t ip;
  uint8_t mac[6];
  int assigned;
};

static struct arp_entry arp_cache[ARP_CACHE_SIZE];
extern uint8_t mac_address[6]; //  MAC address of the local interface (currently
                               //  RTL8139 driver)
extern uint32_t sojeb_os_ip;         // kernel assigned ip

/**
 * handle incoming ARP packets
 */
void arp_handle_packet(void *data, uint32_t len) {
  if (len < sizeof(struct arp_packet)) {
    return;
  }

  struct arp_packet *arp = (struct arp_packet *)data;
  uint16_t op = __builtin_bswap16(arp->oper);

  // save the sender's info in our cache
  for (int i = 0; i < ARP_CACHE_SIZE; i++) {
    if (!arp_cache[i].assigned || arp_cache[i].ip == arp->src_ip) {
      arp_cache[i].ip = arp->src_ip;
      memcpy(arp_cache[i].mac, arp->src_mac, 6);
      arp_cache[i].assigned = 1;
      break;
    }
  }

  // if it's a request for our IP, send a reply
  if (op == ARP_OP_REQUEST && arp->dest_ip == sojeb_os_ip) {
    struct arp_packet reply;

    reply.htype = __builtin_bswap16(ARP_HTYPE_ETHERNET);
    reply.ptype = __builtin_bswap16(ARP_PTYPE_IPV4);
    reply.hlen = 6;
    reply.plen = 4;
    reply.oper = __builtin_bswap16(ARP_OP_REPLY);

    memcpy(reply.src_mac, mac_address, 6);
    reply.src_ip = sojeb_os_ip;

    memcpy(reply.dest_mac, arp->src_mac, 6);
    reply.dest_ip = arp->src_ip;

    // send back to the sender's MAC via Ethernet
    ethernet_send_packet(arp->src_mac, ETHERTYPE_ARP, &reply,
                         sizeof(struct arp_packet));
  }
}

/**
 * send a broadcast ARP request: "who has this IP?"
 */
void arp_send_request(uint32_t target_ip) {
  struct arp_packet request;
  uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  request.htype = __builtin_bswap16(ARP_HTYPE_ETHERNET);
  request.ptype = __builtin_bswap16(ARP_PTYPE_IPV4);
  request.hlen = 6;
  request.plen = 4;
  request.oper = __builtin_bswap16(ARP_OP_REQUEST);

  memcpy(request.src_mac, mac_address, 6);
  request.src_ip = sojeb_os_ip;

  memset(request.dest_mac, 0, 6); // unknown dest MAC
  request.dest_ip = target_ip;

  ethernet_send_packet(broadcast_mac, ETHERTYPE_ARP, &request,
                       sizeof(struct arp_packet));
}

/**
 * look up a MAC address in the cache
 */
uint8_t *arp_lookup(uint32_t ip) {
  for (int i = 0; i < ARP_CACHE_SIZE; i++) {
    if (arp_cache[i].assigned && arp_cache[i].ip == ip) {
      return arp_cache[i].mac;
    }
  }
  return NULL;
}
