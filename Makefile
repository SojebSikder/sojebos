# Compiler and linker
CC = gcc
LD = ld
CFLAGS = -ffreestanding -m32 -Wall -Wextra -O2
LDFLAGS = -m elf_i386 -T linker.ld

# Sources (now in src/)
SRCS_C = $(wildcard src/*.c)
SRCS_S = $(wildcard src/*.S)

# Objects (put objects in build/ directory)
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
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly sources
build/%.o: src/%.S
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

# Build ISO
$(ISO_IMG): $(KERNEL_BIN) $(GRUB_CFG)
	@echo "Creating ISO..."
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/
	cp $(GRUB_CFG) $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_IMG) $(ISO_DIR)

# Run in QEMU
run: $(ISO_IMG)
	qemu-system-i386 -cdrom $(ISO_IMG) -m 512M

# Clean build artifacts
clean:
	rm -rf build $(OBJS) $(KERNEL_BIN) $(ISO_DIR) $(ISO_IMG)
