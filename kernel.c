#include <stdint.h>

unsigned char inb(unsigned short port);

#define VIDEO_MEMORY 0xB8000
#define WHITE_ON_BLACK 0x0F

volatile uint16_t* video = (uint16_t*) VIDEO_MEMORY;
int cursor = 0;
int shift_pressed = 0; // Track if Shift is held down

// Standard ASCII Map
char scancode_map[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// Shifted ASCII Map
char scancode_map_shift[] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

void putchar(char c) {
    if (c == '\n') {
        cursor += 80 - (cursor % 80);
    } else if (c == '\b') { // Basic Backspace support
        if (cursor > 0) {
            cursor--;
            video[cursor] = (WHITE_ON_BLACK << 8) | ' ';
        }
    } else {
        video[cursor++] = (WHITE_ON_BLACK << 8) | c;
    }
}

void print(const char* str) {
    while (*str) putchar(*str++);
}

void print_number(int num) {
    char buffer[16];
    int i = 0;
    if (num == 0) { putchar('0'); return; }
    if (num < 0) { putchar('-'); num = -num; }
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    while (i--) putchar(buffer[i]);
}

void clear_screen() {
    for (int i = 0; i < 80 * 25; i++) {
        video[i] = (WHITE_ON_BLACK << 8) | ' ';
    }
    cursor = 0;
}

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

unsigned char get_scancode() {
    while (!(inb(0x64) & 1));
    return inb(0x60);
}

int atoi(char* str) {
    int result = 0;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

void __attribute__((section(".text.kernel_main"))) kernel_main() {
    clear_screen();
    print("SojebOS Calculator\n");
    print("Example: 12+5 or 10*2\n\n");

    char input[64];
    int idx = 0;

    while (1) {
        unsigned char sc = get_scancode();

        // Handle Shift Key (Left Shift: 0x2A, Right Shift: 0x36)
        if (sc == 0x2A || sc == 0x36) {
            shift_pressed = 1;
            continue;
        }
        // Handle Shift Release (Press code + 0x80)
        if (sc == 0xAA || sc == 0xB6) {
            shift_pressed = 0;
            continue;
        }
        // Ignore all other key release events (bit 7 is set)
        if (sc & 0x80) continue;

        // Map scancode to ASCII based on shift state
        char c = 0;
        if (sc < 58) {
            c = shift_pressed ? scancode_map_shift[sc] : scancode_map[sc];
        }

        if (!c) continue;

        if (c == '\n') {
            putchar('\n');
            input[idx] = 0;

            // Simple parsing: Find the operator
            int a = atoi(input);
            char* op_ptr = input;
            while (*op_ptr && (*op_ptr >= '0' && *op_ptr <= '9')) op_ptr++;

            char op = *op_ptr;
            int b = atoi(op_ptr + 1);
            int result = 0;

            if (op == '+') result = a + b;
            else if (op == '-') result = a - b;
            else if (op == '*') result = a * b;
            else if (op == '/') result = b ? a / b : 0;

            print("= ");
            print_number(result);
            print("\n\n");

            idx = 0;
        } else if (c == '\b') {
            if (idx > 0) {
                idx--;
                putchar('\b');
            }
        } else {
            if (idx < 63) {
                putchar(c);
                input[idx++] = c;
            }
        }
    }
}
