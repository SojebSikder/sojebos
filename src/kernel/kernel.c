#include "../apps/apps.h"
#include "../drivers/console.h"
#include "../lib/string.h"

void __attribute__((section(".text.kernel_main"))) kernel_main() {
  console_clear();
  console_print("Welcome to SojebOS\n");
  console_print("Type app name to launch or 'exit' to quit\n\n");

  // Register apps
  register_app("hello", hello_app);
  register_app("calc", calculator_app);
  register_app("clear", clear_app);

  char input[32];
  while (1) {
    console_print("Apps:\n");
    for (int i = 0; i < app_count; i++) {
      console_print("  ");
      console_print(apps[i].name);
      console_print("\n");
    }
    console_print("> ");
    console_read_line(input, 32);

    if (strcmp(input, "exit") == 0) {
      console_print("Exiting SojebOS shell...\n");
      break;
    }

    // Search for app by name
    int found = 0;
    for (int i = 0; i < app_count; i++) {
      if (strcmp(input, apps[i].name) == 0) {
        apps[i].func();
        found = 1;
        break;
      }
    }

    if (!found) {
      console_print("Command not found!\n");
    }

    console_print("\nReturning to shell...\n\n");
  }
}
