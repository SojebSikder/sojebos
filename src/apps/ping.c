#include "../kernel/drivers/console.h"
#include "../kernel/drivers/net/rtl8139.h"
#include "../kernel/libc/string.h"
#include "../kernel/memory/memory.h"
#include "../kernel/net/icmp.h"

void command_ping(int argc, char *argv[]) {
  if (argc < 2) {
    console_print("Usage: ping <dest_ip>\n");
    return;
  }

  uint32_t dest_ip = parse_ip(argv[1]);

  console_printf("PINGing %s...\n", argv[1]);

  struct icmp_header ping_req;
  ping_req.type = ICMP_TYPE_ECHO_REQUEST;
  ping_req.code = 0;
  ping_req.id = __builtin_bswap16(1234);    // Arbitrary ID
  ping_req.sequence = __builtin_bswap16(1); // Sequence number
  ping_req.checksum = 0;

  // In a simple ping, the payload can be empty or "Hello"
  char *payload = "SojebOS Ping";
  uint32_t payload_len = strlen(payload);

  // We need to calculate checksum over header + payload
  // We'll create a temp buffer to calculate it correctly
  uint32_t total_len = sizeof(struct icmp_header) + payload_len;
  uint8_t *temp = kmalloc(total_len);
  memcpy(temp, &ping_req, sizeof(struct icmp_header));
  memcpy(temp + sizeof(struct icmp_header), payload, payload_len);

  ((struct icmp_header *)temp)->checksum = net_checksum(temp, total_len);

  // Send via IPv4
  ipv4_send(dest_ip, IPV4_PROTO_ICMP, temp, total_len);

  kfree(temp);

  console_print("Packet sent. Waiting for response...\n");

  // A simple delay loop that keeps the hardware alive
  for (int i = 0; i < 2000; i++) {
    rtl8139_poll();
  }
}
