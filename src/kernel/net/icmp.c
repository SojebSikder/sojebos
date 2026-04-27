#include "icmp.h"
#include "../drivers/console.h"
#include "../libc/string.h"
#include "ipv4.h"

/**
 * Processes incoming ICMP packets and responds to Echo Requests (Pings)
 */
void icmp_handle(struct ipv4_header *ip_hdr, void *data, uint32_t len) {
  if (len < sizeof(struct icmp_header))
    return;

  struct icmp_header *icmp = (struct icmp_header *)data;

  // We only care about Echo Requests (Type 8)
  if (icmp->type == ICMP_TYPE_ECHO_REQUEST) {
    icmp->type = ICMP_TYPE_ECHO_REPLY;
    icmp->code = 0;
    icmp->checksum = 0; // Reset for calculation

    // Recalculate ICMP Checksum
    icmp->checksum = net_checksum(icmp, len);

    // Send the response back using the IPv4 layer
    // We swap the destination: the original sender becomes our target
    ipv4_send(ip_hdr->src_ip, IPV4_PROTO_ICMP, data, len);
  } else if (icmp->type == ICMP_TYPE_ECHO_REPLY) {
    console_printf("Received reply from %x: icmp_seq=%d\n",
                   __builtin_bswap32(ip_hdr->src_ip),
                   __builtin_bswap16(icmp->sequence));
  }
}
