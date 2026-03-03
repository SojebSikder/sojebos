#include "apps.h"
#include "console.h"

ConsoleApp apps[MAX_APPS];
int app_count = 0;

void register_app(const char *name, AppFunc func) {
  if (app_count < MAX_APPS) {
    apps[app_count].name = name;
    apps[app_count].func = func;
    app_count++;
  }
}

// Calculator App
void calculator_app() {
  console_print("Calculator App\n");
  console_print("Example: 12+5\n\n");
  char input[64];
  while (1) {
    console_read_line(input, 64);
    if (input[0] == 'e' && input[1] == 'x' && input[2] == 'i' &&
        input[3] == 't')
      return;
    int a = atoi_simple(input);
    const char *op_ptr = input;
    while (*op_ptr && (*op_ptr >= '0' && *op_ptr <= '9'))
      op_ptr++;
    char op = *op_ptr;
    int b = atoi_simple(op_ptr + 1);
    int result = 0;
    switch (op) {
    case '+':
      result = a + b;
      break;
    case '-':
      result = a - b;
      break;
    case '*':
      result = a * b;
      break;
    case '/':
      result = b ? a / b : 0;
      break;
    }
    console_print("= ");
    char buf[16];
    int temp = result, i = 0;
    if (temp == 0) {
      console_putchar('0');
      console_print("\n\n");
      continue;
    }
    if (temp < 0) {
      console_putchar('-');
      temp = -temp;
    }
    while (temp > 0) {
      buf[i++] = '0' + (temp % 10);
      temp /= 10;
    }
    while (i--)
      console_putchar(buf[i]);
    console_print("\n\n");
  }
}

// Hello App
void hello_app() {
  console_print("Hello App\n");
  console_print("Type something or 'exit' to leave.\n");
  char input[64];
  while (1) {
    console_read_line(input, 64);
    if (input[0] == 'e' && input[1] == 'x' && input[2] == 'i' &&
        input[3] == 't')
      return;
    console_print("You typed: ");
    console_print(input);
    console_print("\n");
  }
}
