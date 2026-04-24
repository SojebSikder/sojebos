#include "rtl8139.h"
#include "../io.h"
#include "../../libc/string.h"
#include "../../memory/memory.h"
#include "../../net/ethernet.h"

// Hardware Constants
#define RX_BUF_SIZE 8192
#define RX_TOTAL_SIZE (RX_BUF_SIZE + 16 + 1500) // Padding for wrap-around
#define TX_DESC_COUNT 4

static uint8_t *rx_buffer;
static uint32_t rx_offset = 0;
static uint8_t current_tx_descriptor = 0;

uint32_t io_base;
uint8_t mac_address[6];

// Helper: Convert virtual address to physical (Critical for DMA)
// Replace 'get_phys_addr' with your kernel's specific paging function.
uint32_t to_phys(void *addr) {
    return (uint32_t)addr;
}

void rtl8139_init(uint32_t pci_io_base) {
    io_base = pci_io_base;

    // Allocate RX Buffer (Must be physically contiguous)
    rx_buffer = (uint8_t *)kmalloc(RX_TOTAL_SIZE);
    memset(rx_buffer, 0, RX_TOTAL_SIZE);

    // Power ON
    outb(io_base + 0x52, 0x00); // CONFIG1

    // Software Reset
    outb(io_base + 0x37, 0x10); // COMMAND
    while ((inb(io_base + 0x37) & 0x10) != 0);

    // Set Receive Buffer Address
    outl(io_base + 0x30, to_phys(rx_buffer)); // RBSTART

    // Setup Interrupts (ROK | TOK)
    outw(io_base + 0x3C, 0x0005); // IMR

    // Configure Receive Mode (RCR)
    // Accept: Broadcast(bit3), Multicast(bit2), MyMAC(bit1)
    // Wrap: bit 7 (Allows writing past 8K end to avoid packet clipping)
    outl(io_base + 0x44, 0x0E | (1 << 7));

    // Enable RX and TX
    outb(io_base + 0x37, 0x0C);

    // Store MAC Address
    for (int i = 0; i < 6; i++) {
        mac_address[i] = inb(io_base + 0x00 + i);
    }
}

void rtl8139_send_packet(void *data, uint32_t len) {
    // Determine which status and address register to use (0-3)
    uint32_t status_reg = io_base + 0x10 + (current_tx_descriptor * 4);
    uint32_t addr_reg   = io_base + 0x20 + (current_tx_descriptor * 4);

    // Set the physical address of the data
    outl(addr_reg, to_phys(data));

    // Set the length and clear OWN bit (starts transmission)
    // The packet must be at least 60 bytes (Ethernet minimum)
    uint32_t transmit_size = (len < 60) ? 60 : len;
    outl(status_reg, transmit_size);

    // Wait for TOK (Transmit OK)
    // Note: In a production driver, you'd use an IRQ here instead of a busy-wait loop
    while (!(inw(io_base + 0x3E) & (1 << (current_tx_descriptor + 2))));

    // Acknowledge the bit in ISR
    outw(io_base + 0x3E, (1 << (current_tx_descriptor + 2)));

    // Advance to next descriptor slot
    current_tx_descriptor = (current_tx_descriptor + 1) % TX_DESC_COUNT;
}

void rtl8139_poll() {
    // While the "Buffer Empty" bit in Command Register is 0
    while (!(inb(io_base + 0x37) & 0x01)) {

        uint16_t *header_ptr = (uint16_t *)(rx_buffer + rx_offset);
        uint16_t status = header_ptr[0];
        uint16_t packet_len = header_ptr[1];

        if (status & 0x01) { // ROK: Receive OK
            // Data starts 4 bytes after the header
            uint8_t *packet_data = (uint8_t *)(rx_buffer + rx_offset + 4);

            // Send to Ethernet Stack (Minus 4 bytes for CRC)
            ethernet_handle_packet(packet_data, packet_len - 4);

            // Update offset: Header(4) + Data + CRC(4)
            // Align to 4-byte boundary
            rx_offset = (rx_offset + packet_len + 4 + 3) & ~3;

            // Handle Ring wrap
            if (rx_offset >= RX_BUF_SIZE) {
                rx_offset %= RX_BUF_SIZE;
            }

            // Write CAPR (Current Address of Packet Read)
            // -0x10 is the required "magic" offset to prevent overflow errors
            outw(io_base + 0x38, rx_offset - 0x10);
        }

        // Clear the ROK bit in the ISR
        outw(io_base + 0x3E, 0x01);
    }
}
