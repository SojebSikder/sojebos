#ifndef APPS_H
#define APPS_H

typedef void (*AppFunc)();

typedef struct {
  const char *name;
  AppFunc func;
} ConsoleApp;

#define MAX_APPS 8
extern ConsoleApp apps[MAX_APPS];
extern int app_count;

void register_app(const char *name, AppFunc func);

// Predefined apps
void calculator_app();
void hello_app();
void clear_app();


#endif
