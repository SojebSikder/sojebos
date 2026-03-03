#include <stdint.h>

unsigned char inb(unsigned short port);

#define VIDEO_MEMORY 0xB8000
#define WHITE_ON_BLACK 0x0F

volatile uint16_t* video = (uint16_t*) VIDEO_MEMORY;
int cursor = 0;

void putchar(char c) {
    if (c == '\n') {
        cursor += 80 - (cursor % 80);
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

    if (num == 0) {
        putchar('0');
        return;
    }

    while (num > 0) {
        buffer[i++] = '0' + num % 10;
        num /= 10;
    }

    while (i--)
        putchar(buffer[i]);
}

void clear_screen() {
    for (int i = 0; i < 80 * 25; i++) {
        video[i] = (WHITE_ON_BLACK << 8) | ' ';
    }
    cursor = 0;
}

char get_key() {
    unsigned char sc;
    do {
        while (!(inb(0x64) & 1)); // Wait for Keyboard Controller to be ready
        sc = inb(0x60);           // Read the scancode
    } while (sc & 0x80);          // 0x80 bit is set when a key is released; skip it
    return sc;
}

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

char scancode_to_ascii(unsigned char sc) {
    char map[] = {
        0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
        0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
        'z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
    };

    if (sc > 57) return 0;
    return map[sc];
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
    print("\nSojebOS Calculator\n");
    print("Type like: 5+3\n\n");

    char input[32];
    int idx = 0;

    while (1) {
        unsigned char sc = get_key();
        char c = scancode_to_ascii(sc);

        if (!c) continue;

        if (c == '\n') {
            putchar('\n');
            input[idx] = 0;

            int a = atoi(input);
            char* op = input;

            while (*op && (*op >= '0' && *op <= '9')) op++;

            int b = atoi(op + 1);

            int result = 0;

            if (*op == '+') result = a + b;
            if (*op == '-') result = a - b;
            if (*op == '*') result = a * b;
            if (*op == '/') result = b ? a / b : 0;

            print("= ");
            print_number(result);
            print("\n\n");

            idx = 0;
        }
        else {
            putchar(c);
            input[idx++] = c;
        }
    }
}
