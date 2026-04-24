#include "icmp.h"
#include "ipv4.h"
#include "../libc/string.h"

/**
 * Processes incoming ICMP packets and responds to Echo Requests (Pings)
 */
void icmp_handle(struct ipv4_header* ip_hdr, void* data, uint32_t len) {
    if (len < sizeof(struct icmp_header)) return;

    struct icmp_header* icmp = (struct icmp_header*)data;

    // We only care about Echo Requests (Type 8)
    if (icmp->type == ICMP_TYPE_ECHO_REQUEST) {

        // Prepare the reply header
        // We can modify the incoming header in place for the reply to save memory
        icmp->type = ICMP_TYPE_ECHO_REPLY;
        icmp->code = 0;
        icmp->checksum = 0; // Reset for calculation

        // Recalculate ICMP Checksum
        // The ICMP checksum covers the header AND the payload (the whole 'len')
        icmp->checksum = net_checksum(icmp, len);

        // Send the response back using the IPv4 layer
        // We swap the destination: the original sender becomes our target
        ipv4_send(ip_hdr->src_ip, IPV4_PROTO_ICMP, data, len);
    }
}
