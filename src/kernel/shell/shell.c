/**
 * Shell
 *
 * Command line interface
 */

#include "../../apps/apps.h"
#include "../drivers/console.h"
#include "../libc/string.h"
#include "../drivers/net/rtl8139.h"


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
    for (int i = 0; i < app_count; i++) {
      if (strcmp(argv[0], apps[i].name) == 0) {
        apps[i].func(argc, argv);
        found = 1;
        break;
      }
    }

    if (!found) {
      console_print("Command not found!\n");
    }
  }
}
