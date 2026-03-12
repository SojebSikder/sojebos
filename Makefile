# Compiler and linker
CC = gcc
LD = ld
CFLAGS = -ffreestanding -m32 -Wall -Wextra -O2
LDFLAGS = -m elf_i386 -T linker.ld

# Source files (recursive)
SRCS_C = $(shell find src -name "*.c")
SRCS_S = $(shell find src -name "*.S")

# Object files (mirror src structure inside build/)
OBJS_C = $(patsubst src/%.c, build/%.o, $(SRCS_C))
OBJS_S = $(patsubst src/%.S, build/%.o, $(SRCS_S))
OBJS = $(OBJS_C) $(OBJS_S)

# Kernel binary
KERNEL_BIN = kernel.bin

# ISO
ISO_DIR = iso
ISO_IMG = sojeb-os.iso
GRUB_CFG = boot/grub/grub.cfg

.PHONY: all run clean

# Default target
all: $(ISO_IMG)

# Link kernel
$(KERNEL_BIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Compile C sources
build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly sources
build/%.o: src/%.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Build ISO
$(ISO_IMG): $(KERNEL_BIN) $(GRUB_CFG)
	@echo "Creating ISO..."
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/
	cp $(GRUB_CFG) $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_IMG) $(ISO_DIR)


# Variables
DISK_IMG = disk.img
SOURCE_DIR = disk

# Mark targets that aren't actual files
.PHONY: createdisk addfiles cleandisk

# Create the disk only if it doesn't exist
$(DISK_IMG):
	@echo "Creating 64MB blank FAT32 disk image..."
	dd if=/dev/zero of=$(DISK_IMG) bs=1M count=64
	mkfs.vfat -F 32 $(DISK_IMG)

createdisk: $(DISK_IMG)

# Add files from the host 'disk' directory to the image root
addfiles: $(DISK_IMG)
	@echo "Copying contents of $(SOURCE_DIR) to $(DISK_IMG)..."
	@if [ -d $(SOURCE_DIR) ]; then \
		mcopy -i $(DISK_IMG) -s $(SOURCE_DIR)/* ::/; \
	else \
		echo "Error: Directory $(SOURCE_DIR) not found."; \
	fi

cleandisk:
	rm -f $(DISK_IMG)




# Run in QEMU
run: $(ISO_IMG)
	# qemu-system-i386 -cdrom $(ISO_IMG) -m 512M
	qemu-system-i386 -boot d -cdrom $(ISO_IMG) -drive format=raw,file=$(DISK_IMG) -m 512M

# Clean build artifacts
clean:
	rm -rf build $(KERNEL_BIN) $(ISO_DIR) $(ISO_IMG)
