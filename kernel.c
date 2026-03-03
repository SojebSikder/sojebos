#include "apps.h"
#include "console.h"

void __attribute__((section(".text.kernel_main"))) kernel_main() {
  console_clear();
  console_print("Welcome SojebOS\n");
  console_print("Type app number to launch or 'exit' to quit\n\n");

  // Register apps
  register_app("Calculator", calculator_app);
  register_app("Hello", hello_app);

  char input[16];
  while (1) {
    console_print("Apps:\n");
    for (int i = 0; i < app_count; i++) {
      console_print("  ");
      console_putchar('0' + i);
      console_print(": ");
      console_print(apps[i].name);
      console_print("\n");
    }
    console_print("> ");
    console_read_line(input, 16);
    int choice = input[0] - '0';
    if (choice >= 0 && choice < app_count)
      apps[choice].func();
    console_print("\nReturning to shell...\n\n");
  }
}
