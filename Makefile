CC = gcc
LD = ld
CFLAGS = -ffreestanding -m32 -Wall -Wextra -O2
LDFLAGS = -m elf_i386 -T linker.ld

KERNEL_C = kernel.c
ENTRY_S = entry.S
KERNEL_O = kernel.o
ENTRY_O = entry.o
KERNEL_BIN = kernel.bin

ISO_DIR = iso
ISO_IMG = sojeb-os.iso
GRUB_CFG = boot/grub/grub.cfg

.PHONY: all run clean

all: $(ISO_IMG)

# Build kernel
$(KERNEL_BIN): $(ENTRY_O) $(KERNEL_O)
	$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(ENTRY_O) $(KERNEL_O)

$(KERNEL_O): $(KERNEL_C)
	$(CC) $(CFLAGS) -c $(KERNEL_C) -o $(KERNEL_O)

$(ENTRY_O): $(ENTRY_S)
	$(CC) $(CFLAGS) -c $(ENTRY_S) -o $(ENTRY_O)

# Build ISO
$(ISO_IMG): $(KERNEL_BIN) $(GRUB_CFG)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/
	cp $(GRUB_CFG) $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_IMG) $(ISO_DIR)

# Run in QEMU
run: $(ISO_IMG)
	qemu-system-i386 -cdrom $(ISO_IMG)

# Clean
clean:
	rm -rf $(KERNEL_O) $(ENTRY_O) $(KERNEL_BIN) $(ISO_DIR) $(ISO_IMG)
