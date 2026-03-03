all:
	nasm -f bin boot.asm -o boot.bin
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c kernel.c -o kernel.o
	ld -m elf_i386 -T linker.ld kernel.o -o kernel.bin --oformat binary
	cat boot.bin kernel.bin > os.bin
	# Pad the image to at least 20 sectors (10,240 bytes)
	truncate -s +10K os.bin

run:
	qemu-system-i386 -drive format=raw,file=os.bin
