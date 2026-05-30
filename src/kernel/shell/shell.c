/**
 * Shell
 *
 * Command line interface
 */

#include "../../apps/apps.h"
#include "../drivers/console.h"
#include "../drivers/net/rtl8139.h"
#include "../libc/string.h"
#include "../libc/vector.h"
#include "../process/process.h"

extern Vector apps_vector;

void shell_run() {
  char input[64];
  char *argv[10]; // support up to 10 arguments

  while (1) {
    rtl8139_poll();

    console_print("> ");
    console_read_line(input, 64);

    // tokenize the input
    int argc = 0;
    char *ptr = input;

    while (*ptr != '\0' && argc < 10) {
      // Skip leading spaces
      while (*ptr == ' ') {
        ptr++;
      }
      if (*ptr == '\0') {
        break;
      }

      argv[argc++] = ptr;

      // Find end of token
      while (*ptr != ' ' && *ptr != '\0') {
        ptr++;
      }
      if (*ptr != '\0') {
        *ptr = '\0'; // Null terminate the argument
        ptr++;
      }
    }

    if (argc == 0) {
      continue;
    }

    if (strcmp(argv[0], "exit") == 0) {
      console_print("Exiting SojebOS shell...\n");
      break;
    }

    // Search for app by name
    int found = 0;
    for (size_t i = 0; i < apps_vector.size; i++) {
      ConsoleApp *app = (ConsoleApp *)vector_get(&apps_vector, i);

      if (app && strcmp(argv[0], app->name) == 0) {
        app->func(argc, argv);
        found = 1;
        break;
      }
    }

    if (!found) {
      int status = exec_elf(argv[0]);

      if (status >= 0) {
        found = 1;
      } else {
        console_print("Command or file not found!\n");
      }
    }
  }
}
