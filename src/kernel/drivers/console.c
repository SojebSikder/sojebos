#include "console.h"
#include "./io.h"

#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_DATA_PORT 0x60

// Video Memory
volatile uint16_t *video = (uint16_t *)VIDEO_MEMORY;
Console console = {0, 0};

// Scancode Maps
static char scancode_map[] = {
    0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
    'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' '};
static char scancode_map_shift[] = {
    0,   27,  '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' '};

unsigned char get_scancode() {
  while (!(inb(KEYBOARD_STATUS_PORT) & 1))
    ;
  return inb(KEYBOARD_DATA_PORT);
}

// Scroll the screen up by one line
static void scroll_up() {
  for (int y = 1; y < CONSOLE_HEIGHT; y++) {
    for (int x = 0; x < CONSOLE_WIDTH; x++) {
      video[(y - 1) * CONSOLE_WIDTH + x] = video[y * CONSOLE_WIDTH + x];
    }
  }
  // Clear the last line
  for (int x = 0; x < CONSOLE_WIDTH; x++) {
    video[(CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH + x] =
        (WHITE_ON_BLACK << 8) | ' ';
  }
}

// Console Functions
void console_putchar(char c) {
  if (c == '\n') {
    console.cursor += CONSOLE_WIDTH - (console.cursor % CONSOLE_WIDTH);
  } else if (c == '\b') {
    if (console.cursor > 0) {
      console.cursor--;
      video[console.cursor] = (WHITE_ON_BLACK << 8) | ' ';
    }
  } else {
    video[console.cursor++] = (WHITE_ON_BLACK << 8) | c;
  }

  // Scroll if cursor goes beyond the last line
  if (console.cursor >= CONSOLE_WIDTH * CONSOLE_HEIGHT) {
    scroll_up();
    console.cursor -= CONSOLE_WIDTH;
  }
}

void console_print(const char *str) {
  while (*str) {
    console_putchar(*str++);
  }
}

void console_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            i++;
            if (fmt[i] == 's') {
                char *str = va_arg(args, char *);
                console_print(str);
            } else if (fmt[i] == 'c') {
                char c = (char)va_arg(args, int);
                console_putchar(c);
            } else if (fmt[i] == 'd') {
                int num = va_arg(args, int);

                char buf[16];
                int j = 0;

                if (num == 0) {
                    console_putchar('0');
                    continue;
                }

                if (num < 0) {
                    console_putchar('-');
                    num = -num;
                }

                while (num > 0) {
                    buf[j++] = '0' + (num % 10);
                    num /= 10;
                }

                while (j--) {
                    console_putchar(buf[j]);
                }
            } else {
                console_putchar('%');
                console_putchar(fmt[i]);
            }
        } else {
            console_putchar(fmt[i]);
        }
    }

    va_end(args);
}

void console_clear() {
  for (int i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
    video[i] = (WHITE_ON_BLACK << 8) | ' ';
  }
  console.cursor = 0;
}

char console_get_char() {
  while (1) {
    unsigned char sc = get_scancode();
    if (sc == 0x2A || sc == 0x36) {
      console.shift_pressed = 1;
      continue;
    }
    if (sc == 0xAA || sc == 0xB6) {
      console.shift_pressed = 0;
      continue;
    }
    if (sc & 0x80) {
      continue;
    }
    if (sc < 58) {
      char c =
          console.shift_pressed ? scancode_map_shift[sc] : scancode_map[sc];
      if (c) {
        return c;
      }
    }
  }
}

void console_read_line(char *buffer, int max_len) {
  int idx = 0;
  while (1) {
    char c = console_get_char();
    if (c == '\n') {
      console_putchar('\n');
      buffer[idx] = 0;
      return;
    } else if (c == '\b') {
      if (idx > 0) {
        idx--;
        console_putchar('\b');
      }
    } else if (idx < max_len - 1) {
      buffer[idx++] = c;
      console_putchar(c);
    }
  }
}
