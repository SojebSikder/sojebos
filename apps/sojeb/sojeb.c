// The entry point for the ELF execution engine
void _start() {
    // console_clear();

    // TODO: TRAP/EXIT NOTE:
    // Once console_clear finishes, the CPU will keep executing random memory
    // unless you yield control back to the kernel via a system call.
    // For now, we will just loop indefinitely if you don't have a exit() syscall yet.
    while(1);
}
