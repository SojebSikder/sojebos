#include "../drivers/console.h"

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
