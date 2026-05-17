#ifndef APPS_H
#define APPS_H

#include "../kernel/libc/vector.h"

typedef void (*AppFunc)(int argc, char *argv[]);

typedef struct {
  const char *name;
  AppFunc func;
} ConsoleApp;

extern Vector apps_vector;

void register_app(const char *name, AppFunc func);
void register_all_apps();

#endif
