#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stdarg.h>


#define VIDEO_MEMORY 0xB8000
#define WHITE_ON_BLACK 0x0F
#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 25

typedef struct {
  int cursor;
  int shift_pressed;
} Console;

extern volatile uint16_t *video;
extern Console console;

void console_clear();
void console_putchar(char c);
void console_print(const char *str);
void console_printf(const char *fmt, ...);
char console_get_char();
void console_read_line(char *buffer, int max_len);


#endif
