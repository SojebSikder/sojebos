## Description
SojebOS is a basic operating system

## Screenshots
![Screenshot](./screenshots/screenshot1.png)

## Features
- coreutils: ls, pwd, cd, mkdir, rmdir, cat, write, rm, df, clear
- utils: calculator
- fat32 file system
- Many more and many more to come...


## Build Instructions
```bash
# Install dependencies
sudo apt install build-essential nasm mtools xorriso grub-pc-bin grub-common

# Create disk image (first time only)
make createdisk
# Add files to the disk image
make addfiles
# Build
make
# Run 
make run
```
