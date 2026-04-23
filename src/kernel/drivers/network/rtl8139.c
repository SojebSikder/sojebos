#include "rtl8139.h"
#include "../io.h"
#include "../../memory/memory.h"
#include "../../net/ethernet.h"

static uint8_t *rx_buffer;
static uint32_t rx_offset = 0;
uint32_t io_base;
uint8_t mac_address[6];

/**
 * Initialize the RTL8139 Network Card
 */
void rtl8139_init(uint32_t pci_io_base) {
    io_base = pci_io_base;

    // Allocate the Receive Buffer (8K + 16 bytes + 1.5K for safety)
    // The RTL8139 uses a wrap-around ring buffer.
    rx_buffer = (uint8_t *)kmalloc(8192 + 16 + 1500);

    // Power ON the device (Config 1 register)
    outb(io_base + RTL8139_REG_CONFIG1, 0x00);

    // Software Reset
    outb(io_base + RTL8139_REG_COMMAND, 0x10);
    while ((inb(io_base + RTL8139_REG_COMMAND) & 0x10) != 0) {
        // Wait for reset to complete
    }

    // Set the Receive Buffer address (RBSTART)
    // Note: If you implement paging later, this MUST be the physical address.
    outl(io_base + RTL8139_REG_RBSTART, (uint32_t)rx_buffer);

    // Initialize Interrupts (IMR)
    // we set these bits so the card updates its status regs.
    // 0x0005 = Sets ROK (Receive OK) and TOK (Transmit OK)
    outw(io_base + RTL8139_REG_IMR, 0x0005);

    // Configure Receive Mode (RCR)
    // 0x0F = Accept Broadcast, Multicast, My MAC, and Promiscuous
    // (1 << 7) = Wrap bit (allows the card to write past the end of buffer temporarily)
    outl(io_base + 0x44, 0xf | (1 << 7));

    // Enable Transmitter and Receiver
    outb(io_base + RTL8139_REG_COMMAND, 0x0C);

    // Read MAC Address
    for (int i = 0; i < 6; i++) {
        mac_address[i] = inb(io_base + RTL8139_REG_MAC + i);
    }
}

/**
 * Send a raw Ethernet frame
 */
void rtl8139_send_packet(void *data, uint32_t len) {
    // We use Transmit Descriptor 0 (out of 4 available)
    // Write physical address of data
    outl(io_base + RTL8139_REG_TXADDR, (uint32_t)data);

    // Write length and clear "OWN" bit to trigger transmission
    outl(io_base + RTL8139_REG_TXSTATUS, len);

    // Wait for transfer to complete (polling for TOK)
    while (!(inw(io_base + RTL8139_REG_ISR) & 0x04));

    // Clear TOK bit
    outw(io_base + RTL8139_REG_ISR, 0x04);
}

/**
 * Manually check the card for new packets (Polling Mode)
 */
void rtl8139_poll() {
    // Check if "Buffer Empty" bit (bit 0) is 0
    if (!(inb(io_base + RTL8139_REG_COMMAND) & 0x01)) {

        uint32_t read_ptr = (uint32_t)rx_buffer + rx_offset;

        // Packet Header: [Status 2 bytes][Length 2 bytes]
        uint16_t status = *(uint16_t *)(read_ptr);
        uint16_t packet_len = *(uint16_t *)(read_ptr + 2);

        // Verify ROK (Receive OK) status bit
        if (status & 0x01) {
            uint8_t *packet_data = (uint8_t *)(read_ptr + 4);

            // Dispatch to the Network Stack
            ethernet_handle_packet(packet_data, packet_len);

            // Update offset: Header(4) + Data + CRC(4)
            // Must be 4-byte aligned
            rx_offset = (rx_offset + packet_len + 4 + 3) & ~3;

            // Handle Ring Buffer wrap around
            if (rx_offset >= 8192) {
                rx_offset %= 8192;
            }

            // Update CAPR (Current Address of Packet Read)
            // We subtract 0x10 to prevent buffer overflow conditions
            outw(io_base + RTL8139_REG_CAPR, rx_offset - 0x10);
        }

        // Acknowledge Receive OK
        outw(io_base + RTL8139_REG_ISR, 0x01);
    }
}
