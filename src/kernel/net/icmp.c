#include "icmp.h"
#include "ipv4.h"

void icmp_header(struct ipv4_header *ip_pkt, void *data, uint32_t len) {
  struct icmp_header *icmp = (struct icmp_header *)data;

  if (icmp->type == ICMP_TYPE_ECHO_REQUEST) {
    // prepare reply
    icmp->type = ICMP_TYPE_ECHO_REPLY;
    icmp->checksum = 0;
    icmp->checksum = net_checksum(icmp, len);

    // send back to sender
    ipv4_send(ip_pkt->src_ip, 1, data, len);
  }
}

/**
 * Handles incoming ICMP packets.
 * @param ip_pkt: Pointer to the encapsulating IPv4 header
 * @param data:   Pointer to the start of the ICMP payload
 * @param len:    Length of the ICMP payload
 */
void icmp_handle(struct ipv4_header *ip_pkt, void *data, uint32_t len) {
    // Ensure the packet is at least large enough for an ICMP header
    if (len < sizeof(struct icmp_header)) {
        return;
    }

    struct icmp_header *icmp = (struct icmp_header *)data;

    // Verify Checksum: ICMP checksum covers the entire ICMP message
    uint16_t received_checksum = icmp->checksum;
    icmp->checksum = 0;

    if (received_checksum != net_checksum(data, len)) {
        // Drop packet if checksum is invalid
        return;
    }

    // Restore checksum for the handler logic
    icmp->checksum = received_checksum;

    // Dispatch to the specific ICMP type handler
    icmp_header(ip_pkt, data, len);
}
